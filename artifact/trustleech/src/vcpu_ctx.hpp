#pragma once

#include <sysops.hpp>
#include <assert.hpp>
#include <memory_backed_regs.hpp>
#include <registers.hpp>
#include <types.hpp>


#define DEFINE_RENAME_PURE_MEM_REG(type, field) \
	public: \
		type field() const { \
			return memory_backend.field(); \
		} \
		\
		void set_ ## field(type wrapper) { \
			memory_backend.set_ ## field(wrapper); \
		}


#define DEFINE_PURE_MEM_REG(struct_or_union, field) \
	DEFINE_RENAME_PURE_MEM_REG(struct_or_union field, field)


//
// A register is viable (i.e. potentially alive) if it is potentially
// active/live on a physical CPU register (making access conditional). This can
// either be in an unaltered state (e.g. for E2H vEL2 guests) or translated.
//
// This registers typically affect the operation of vEL2.
//

#define DEFINE_RENAME_VIABLE_VEL2_REG(xlate, el1_type, el1_field, el2_type, el2_field) \
	private: \
		el1_type translate_ ## el2_field(el2_type input) { \
			const auto xlate_casted = static_cast<el1_type (*)(el2_type)>(xlate); \
			return xlate_casted && !hcr_el2().e2h() \
				? xlate_casted(input) \
				: (el1_type) { input.v }; \
		} \
		\
	public: \
		el2_type el2_field() const { \
			if ((!bool(xlate) || hcr_el2().e2h()) && this->is_vel2) { \
				el1_type value = read_ ## el1_field(); \
				return (el2_type) { value.v }; \
			} else { \
				return memory_backend.el2_field(); \
			} \
		} \
		\
		void set_ ## el2_field(el2_type wrapper) { \
			memory_backend.set_ ## el2_field(wrapper); \
			\
			if (this->is_vel2) { \
				write_ ## el1_field(translate_ ## el2_field(wrapper)); \
			} \
		}


#define DEFINE_VIABLE_VEL2_REG(xlate, el1_struct_or_union, el1_field, el2_struct_or_union, el2_field) \
	DEFINE_RENAME_VIABLE_VEL2_REG(xlate, el1_struct_or_union el1_field, el1_field, el2_struct_or_union el2_field, el2_field)


#define DEFINE_VIABLE_VEL2_REG_SAMETYPE(xlate, type, el1_field, el2_field) \
	DEFINE_RENAME_VIABLE_VEL2_REG(xlate, type, el1_field, type, el2_field)

// EL1 regs are mostly viable, but have simplified accessors

#define DEFINE_RENAME_VIABLE_EL1_REG(type, field) \
	public: \
		type field() const { \
			if (is_vel2) { \
				return memory_backend.field(); \
			} else { \
				return read_ ## field(); \
			} \
		} \
		\
		void set_ ## field(type wrapper) { \
			if (is_vel2) { \
				memory_backend.set_ ## field(wrapper); \
			} else { \
				write_ ## field(wrapper); \
			} \
		}


#define DEFINE_VIABLE_EL1_REG(struct_or_union, field) \
	DEFINE_RENAME_VIABLE_EL1_REG(struct_or_union field, field)

//
// Some registers are only ever stored in a live register
//

#define DEFINE_RENAME_PURELY_ALIVE_REG(type, field) \
	public: \
		type field() const { \
			return read_ ## field(); \
		} \
		\
		void set_ ## field(type wrapper) { \
			write_ ## field(wrapper); \
		}


#define DEFINE_PURELY_ALIVE_REG(struct_or_union, field) \
	DEFINE_RENAME_PURELY_ALIVE_REG(struct_or_union field, field)


namespace xlate {
	sctlr translate_sctlr_el2(sctlr);
	xlate_tbl_base translate_ttbr0_el2(xlate_tbl_base);
	tcr_el1 translate_tcr_el2(tcr_el2);
	cntkctl_el1 translate_cnthctl_el2(cnthctl_el2);
	cpacr_el1 translate_cptr_el2(cptr_el2);
}

struct vcpu_ctx {
	memory_backed_regs memory_backend;

	/** Whether we are currently emulating a vEL2
	 *
	 * When this is true, we expect EL1 registers to be stored in
	 * memory_backend, and (viable) EL2 registers to be stored in their
	 * physical EL1 register equivalents.
	 *
	 * On the other hand, when this is false, we expect EL1 registers to be
	 * live in the phsyical EL1 registers, while EL2 registers are stored in
	 * memory_backend.
	 */
	bool is_vel2 = false;
	/** Whether we should switch to vEL2 */
	bool dispatch_vel2 = false;
	/** The current program status register when returning to the guest */
	struct saved_program_status cpsr {};
	/** The current program counter when returning to the guest */
	uintptr_t pc = 0;
	/** The last value we observed for the virtual CNTVOFF_EL2 */
	uint64_t last_cntvoff = 0;

	void save_program_state(
		saved_program_status spsr = read_spsr_el2(),
		uintptr_t saved_pc = read_elr_el2().return_address
	) {
		cpsr = spsr;
		pc = saved_pc;

		assert(!cpsr.is_aarch32() && "AArch32 support not implemented");
	}

	void restore_program_state() {
		assert(!cpsr.is_aarch32());
		assert(cpsr.el() < EL2);

		write_elr_el2(exception_link { pc });
		write_spsr_el2(cpsr);
        isb();
	}

	/** Returns the exception level the vCPU thinks it is running at */
	constexpr exception_level virtual_el() const {
		return is_vel2 && cpsr.el() == EL1
			? EL2 : cpsr.el();
	}

	/** Read EL1 registers that may be modified by a VM to memory
	 *
	 * When dispatching vEL2, we need to read the EL1 registers that are
	 * accessed by a VM to the virtual nested control block.
	 */
	void read_vm_registers();

	/** Write EL1 registers that may be modified by a VM to memory
	 *
	 * When dispatching a VM, we need to write the EL1 registers that were
	 * written to the virtual nested control block by vEL2
	 */
	void write_vm_registers();

	/** Read values of viable registers from physical registers.
	 *
	 * When dispatching a VM, we need to save registers potentially alive in
	 * EL1 registers since they would be overwritten by the VM.
	 */
	void read_viable_regs();

	/** Write (translated) values of viable registers to physical registers.
	 *
	 * This is done to prepare for vEL2 dispatch, since these registers affect
	 * the vEL2 operation.
	 */
	void write_viable_regs();

	//
	// Registers
	//

	// Auxiliary
	DEFINE_PURE_MEM_REG(struct, actlr_el2);
	DEFINE_VIABLE_VEL2_REG(
		nullptr,
		struct, afsr0_el1,
		struct, afsr0_el2
	);
	DEFINE_VIABLE_VEL2_REG(
		nullptr,
		struct, afsr1_el1,
		struct, afsr1_el2
	);
	DEFINE_VIABLE_VEL2_REG(
		nullptr,
		struct, amair_el1,
		struct, amair_el2
	);
	DEFINE_PURE_MEM_REG(struct, hacr_el2);

	// System Control
	DEFINE_VIABLE_VEL2_REG_SAMETYPE(
		xlate::translate_sctlr_el2, union sctlr,
		sctlr_el1, sctlr_el2
	)
	DEFINE_VIABLE_VEL2_REG_SAMETYPE(
		nullptr, union stack_pointer,
		sp_el1, sp_el2
	);
	DEFINE_RENAME_PURE_MEM_REG(union software_thread_id, tpidr_el2);

	// Generic Timer
	DEFINE_RENAME_PURELY_ALIVE_REG(timer_control, cnthp_ctl_el2);
	DEFINE_RENAME_PURELY_ALIVE_REG(timer_cval, cnthp_cval_el2);
	DEFINE_RENAME_PURELY_ALIVE_REG(timer_tval, cnthp_tval_el2);

	DEFINE_RENAME_PURELY_ALIVE_REG(timer_control, cntv_ctl_el0);
	DEFINE_RENAME_PURELY_ALIVE_REG(timer_cval, cntv_cval_el0);
	DEFINE_RENAME_PURELY_ALIVE_REG(timer_tval, cntv_tval_el0);

	DEFINE_RENAME_PURELY_ALIVE_REG(timer_control, cntp_ctl_el0);
	DEFINE_RENAME_PURELY_ALIVE_REG(timer_cval, cntp_cval_el0);
	DEFINE_RENAME_PURELY_ALIVE_REG(timer_tval, cntp_tval_el0);

	// Hypervisor control
	DEFINE_PURE_MEM_REG(struct, hcr_el2);
	DEFINE_PURE_MEM_REG(struct, cntvoff_el2);
	DEFINE_PURE_MEM_REG(struct, hpfar_el2);
	DEFINE_PURE_MEM_REG(struct, hstr_el2);
	DEFINE_RENAME_PURE_MEM_REG(struct mpid, vmpidr_el2);
	DEFINE_RENAME_PURE_MEM_REG(struct pid, vpidr_el2);
	DEFINE_PURE_MEM_REG(struct, vtcr_el2);
	DEFINE_PURE_MEM_REG(struct, vttbr_el2);
	DEFINE_VIABLE_VEL2_REG(
		xlate::translate_cnthctl_el2,
		struct, cntkctl_el1,
		union, cnthctl_el2
	)
	DEFINE_VIABLE_VEL2_REG(
		xlate::translate_cptr_el2,
		struct, cpacr_el1,
		union, cptr_el2
	);

	// MMU
	DEFINE_VIABLE_VEL2_REG_SAMETYPE(
		nullptr, union indirect_mem_attr,
		mair_el1, mair_el2
	);
	DEFINE_VIABLE_VEL2_REG(
		xlate::translate_tcr_el2,
		struct, tcr_el1,
		struct, tcr_el2
	)
	DEFINE_VIABLE_VEL2_REG_SAMETYPE(
		xlate::translate_ttbr0_el2, struct xlate_tbl_base,
		ttbr0_el1, ttbr0_el2
	);

	// Exceptions
	DEFINE_VIABLE_VEL2_REG_SAMETYPE(
		nullptr, union exception_link,
		elr_el1, elr_el2
	)
	DEFINE_VIABLE_VEL2_REG_SAMETYPE(
		nullptr, struct saved_program_status,
		spsr_el1, spsr_el2
	)
	DEFINE_VIABLE_VEL2_REG_SAMETYPE(
		nullptr, union exception_syndrome,
		esr_el1, esr_el2
	)
	DEFINE_VIABLE_VEL2_REG_SAMETYPE(
		nullptr, union faulting_address,
		far_el1, far_el2
	)
	DEFINE_VIABLE_VEL2_REG_SAMETYPE(
		nullptr, union vector_base,
		vbar_el1, vbar_el2
	)

	// Debug
	DEFINE_PURE_MEM_REG(struct, mdcr_el2);

	//
	// EL1 Registers
	//

	// System control
	DEFINE_RENAME_VIABLE_EL1_REG(union sctlr, sctlr_el1);
	DEFINE_VIABLE_EL1_REG(struct, cpacr_el1);
	DEFINE_VIABLE_EL1_REG(struct, cntkctl_el1);

	// Exceptions
	DEFINE_RENAME_VIABLE_EL1_REG(union vector_base, vbar_el1);
	DEFINE_RENAME_VIABLE_EL1_REG(struct saved_program_status, spsr_el1);
	DEFINE_RENAME_VIABLE_EL1_REG(union exception_link, elr_el1);
	DEFINE_RENAME_VIABLE_EL1_REG(union exception_syndrome, esr_el1);
	DEFINE_RENAME_VIABLE_EL1_REG(union faulting_address, far_el1);

	//
	// FEAT_NV2
	//
	DEFINE_PURE_MEM_REG(struct, vncr_el2);

	//
	// FEAT_VHE
	//
	DEFINE_VIABLE_VEL2_REG_SAMETYPE(
		nullptr, ctx_id,
		contextidr_el1, contextidr_el2
	);
	DEFINE_VIABLE_VEL2_REG_SAMETYPE(
		nullptr, xlate_tbl_base,
		ttbr1_el1, ttbr1_el2
	);

	// GIC
	DEFINE_PURELY_ALIVE_REG(struct, ich_vtr_el2);
	DEFINE_PURELY_ALIVE_REG(struct, icc_sre_el2);
	DEFINE_PURELY_ALIVE_REG(struct, ich_elrsr_el2);

	DEFINE_PURE_MEM_REG(struct, ich_hcr_el2);

	//
	// FEAT_HCX
	//
	DEFINE_PURE_MEM_REG(struct, hcrx_el2);

	//
	// FEAT_FGT
	//
	DEFINE_PURE_MEM_REG(struct, hfgrtr_el2);
	DEFINE_PURE_MEM_REG(struct, hfgwtr_el2);
	DEFINE_PURE_MEM_REG(struct, hfgitr_el2);
	DEFINE_PURE_MEM_REG(struct, hdfgrtr_el2);
	DEFINE_PURE_MEM_REG(struct, hdfgwtr_el2);

	// FEAT_FGT && FEAT_AMUv1
	DEFINE_PURE_MEM_REG(struct, hafgrtr_el2);
};

/** Writes a pointer to the current CPU's vcpu_ctx to TPIDR_EL2
 *
 * Returns the old value of TPIDR_EL2
 */
software_thread_id setup_tpidr_el2();

/** Return a reference to this CPU's vcpu_ctx
 *
 * Only valid after `setup_tpidr_el2` has been called.
 */
static inline __attribute__((always_inline)) struct vcpu_ctx& vcpu() {
	struct vcpu_ctx *ptr;
	// Use non-volatile read to allow optimization, as we never write to
	// tpidr_el2 after setup_tpidr_el2
	__asm__ ("mrs %0, tpidr_el2" : "=r"(ptr));
	return *ptr;
}

#undef DEFINE_RENAME_PURE_MEM_REG
#undef DEFINE_PURE_MEM_REG
#undef DEFINE_RENAME_VIABLE_VEL2_REG
#undef DEFINE_VIABLE_VEL2_REG
#undef DEFINE_RENAME_VIABLE_REG_SAMETYPE
#undef DEFINE_RENAME_VIABLE_EL1_REG
#undef DEFINE_VIABLE_EL1_REG
#undef DEFINE_RENAME_PURELY_ALIVE_REG
#undef DEFINE_PURELY_ALIVE_REG
