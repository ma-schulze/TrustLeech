#pragma once

#include <arch.hpp>
#include <assert.hpp>
#include <bitfield.hpp>
#include <exception/types.hpp>
#include <memory.hpp>
#include <paging/types.hpp>
#include <utility.hpp>

#define REGISTER_CHECK(name) assert_size(name, 8)

/****************************************************************************
 * Macros used to create functions to access system registers
 ****************************************************************************/

#define _DEFINE_REGISTER_READ_FUNC(struct_name, reg_name) \
	static inline struct_name read_ ## reg_name() { \
		uint64_t v; \
		__asm__ volatile ("mrs %0, " #reg_name : "=r" (v)); \
		return struct_name { v }; \
	}

#define _DEFINE_REGISTER_WRITE_FUNC(struct_name, reg_name) \
	static inline void write_ ## reg_name(struct_name wrapper) { \
		__asm__ volatile ("msr " #reg_name ", %0" : : "r" (wrapper.v)); \
	}

#define DEFINE_REGISTER_RW_FUNCS(reg_name) \
	_DEFINE_REGISTER_READ_FUNC(reg_name, reg_name) \
	_DEFINE_REGISTER_WRITE_FUNC(reg_name, reg_name)

#define DEFINE_RENAME_REGISTER_RW_FUNCS(struct_name, reg_name) \
	_DEFINE_REGISTER_READ_FUNC(struct_name, reg_name) \
	_DEFINE_REGISTER_WRITE_FUNC(struct_name, reg_name)

/****************************************************************************
 * Utility
 ****************************************************************************/

#define DEFINE_DUMMY_REGISTER(name) \
	struct name { \
		BITFIELD_DATA_UINT64(0); \
	}; \
	\
	DEFINE_REGISTER_RW_FUNCS(name); \
	\
	REGISTER_CHECK(name);

/****************************************************************************
 * System register definitions
 ****************************************************************************/

union sctlr {
	BITFIELD_DATA_UINT64(0x30C00900);

	BITFIELD_FIELD_RW(bool, mmu, 0, 1);
	BITFIELD_FIELD_RW(bool, alignment_check, 1, 1);
	BITFIELD_FIELD_RW(bool, data_cacheability, 2, 1);
	BITFIELD_FIELD_RW(bool, sp_alignment_check, 3, 1);
	BITFIELD_FIELD_RW(bool, sa0, 4, 1);
	BITFIELD_FIELD_RW(bool, cp15ben, 5, 1);
	BITFIELD_FIELD_RW(bool, naa, 6, 1);
	BITFIELD_FIELD_RW(bool, itd, 7, 1);
	BITFIELD_FIELD_RW(bool, sed, 8, 1);
	BITFIELD_FIELD_RW(bool, enrctx, 10, 1);
	BITFIELD_FIELD_RW(bool, eos, 11, 1);
	BITFIELD_FIELD_RW(bool, instruction_cacheability, 12, 1);
	BITFIELD_FIELD_RW(bool, endb, 13, 1);
	BITFIELD_FIELD_RW(bool, dze, 14, 1);
	BITFIELD_FIELD_RW(bool, uct, 15, 1);
	BITFIELD_FIELD_RW(bool, ntwi, 16, 1);
	BITFIELD_FIELD_RW(bool, ntwe, 18, 1);
	BITFIELD_FIELD_RW(bool, write_execute_never, 19, 1);
	BITFIELD_FIELD_RW(bool, tscxt, 20, 1);
	BITFIELD_FIELD_RW(bool, iesb, 21, 1);
	BITFIELD_FIELD_RW(bool, eis, 22, 1);
	BITFIELD_FIELD_RW(bool, span, 23, 1);
	BITFIELD_FIELD_RW(bool, e0e, 24, 1);
	BITFIELD_FIELD_RW(bool, big_endian, 25, 1);
	BITFIELD_FIELD_RW(bool, uci, 26, 1);
	BITFIELD_FIELD_RW(bool, enda, 27, 1);
	BITFIELD_FIELD_RW(bool, ntlsmd, 28, 1);
	BITFIELD_FIELD_RW(bool, lsmaoe, 29, 1);
	BITFIELD_FIELD_RW(bool, enib, 30, 1);
	BITFIELD_FIELD_RW(bool, enia, 31, 1);
	BITFIELD_FIELD_RW(bool, cmow, 32, 1);
	BITFIELD_FIELD_RW(bool, mscen, 33, 1);
	BITFIELD_FIELD_RW(bool, enfpm, 34, 1);
	BITFIELD_FIELD_RW(bool, bt0, 35, 1);
	BITFIELD_FIELD_RW(bool, itfsb, 37, 1);
	BITFIELD_FIELD_RW(uint8_t, tcf0, 38, 2);
	BITFIELD_FIELD_RW(uint8_t, tcf, 40, 2);
	BITFIELD_FIELD_RW(bool, ata0, 42, 1);
	BITFIELD_FIELD_RW(bool, ata, 43, 1);
	BITFIELD_FIELD_RW(bool, dssbs, 44, 1);
	BITFIELD_FIELD_RW(bool, tweden, 45, 1);
	BITFIELD_FIELD_RW(uint8_t, twedel, 46, 4);
	BITFIELD_FIELD_RW(bool, tmt0, 50, 1);
	BITFIELD_FIELD_RW(bool, tmt, 51, 1);
	BITFIELD_FIELD_RW(bool, tme0, 52, 1);
	BITFIELD_FIELD_RW(bool, tme, 53, 1);
	BITFIELD_FIELD_RW(bool, enasr, 54, 1);
	BITFIELD_FIELD_RW(bool, enas0, 55, 1);
	BITFIELD_FIELD_RW(bool, enals, 56, 1);
	BITFIELD_FIELD_RW(bool, epan, 57, 1);
	BITFIELD_FIELD_RW(bool, tcso0, 58, 1);
	BITFIELD_FIELD_RW(bool, tcso, 59, 1);
	BITFIELD_FIELD_RW(bool, entp2, 60, 1);
	BITFIELD_FIELD_RW(bool, nmi, 61, 1);
	BITFIELD_FIELD_RW(bool, spintmask, 62, 1);
	BITFIELD_FIELD_RW(bool, tidcp, 63, 1);

	struct el1 {
		BITFIELD_DATA_UINT64(0x0);

		BITFIELD_FIELD_RW(bool, uma, 9, 1);
		BITFIELD_FIELD_RW(bool, bt1, 36, 1);
	} el1;

	static constexpr sctlr for_el1() {
		return (sctlr {})
			.with_tscxt(true)
			.with_itd(true);
	}

	static constexpr sctlr for_el2() {
		return (sctlr {})
			.with_sa0(true)
			.with_cp15ben(true)
			.with_ntwi(true)
			.with_ntwe(true);
	}

	struct el2 {
		BITFIELD_DATA_UINT64(0x0);

		BITFIELD_FIELD_RW(bool, bt, 36, 1);
	} el2;
};

DEFINE_RENAME_REGISTER_RW_FUNCS(sctlr, sctlr_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(sctlr, sctlr_el2);

REGISTER_CHECK(sctlr);

union stack_pointer {
	BITFIELD_DATA_UINT64(0x0);

	uintptr_t sp;
};

DEFINE_RENAME_REGISTER_RW_FUNCS(stack_pointer, sp_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(stack_pointer, sp_el1);

REGISTER_CHECK(stack_pointer);

struct ctx_id {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(uint32_t, procid, 0, 32);
};

DEFINE_RENAME_REGISTER_RW_FUNCS(ctx_id, contextidr_el1);

REGISTER_CHECK(ctx_id);

struct cpacr_el1 {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(fp_trap, zen, 16, 2);
	BITFIELD_FIELD_RW(fp_trap, fpen, 20, 2);
	BITFIELD_FIELD_RW(fp_trap, smen, 24, 2);
	BITFIELD_FIELD_RW(bool, tta, 28, 1);
	BITFIELD_FIELD_RW(bool, e0poe, 29, 1);
};

DEFINE_REGISTER_RW_FUNCS(cpacr_el1);

REGISTER_CHECK(cpacr_el1);

union software_thread_id {
	BITFIELD_DATA_UINT64(0x0);

	uintptr_t thread_id;
};

DEFINE_RENAME_REGISTER_RW_FUNCS(software_thread_id, tpidr_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(software_thread_id, tpidr_el1);

REGISTER_CHECK(software_thread_id);

//
// MMU
//

struct tcr_el1 {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(uint8_t, t0sz, 0, 6);
	BITFIELD_FIELD_RW(bool, epd0, 7, 1);
	BITFIELD_FIELD_RW(memory::cacheability, inner_cacheability, 8, 2);
	BITFIELD_FIELD_RW(memory::cacheability, outer_cacheability, 10, 2);
	BITFIELD_FIELD_RW(memory::shareability, shareability, 12, 2);
	BITFIELD_FIELD_RW(paging::granule_size, granule_size, 14, 2);
	BITFIELD_FIELD_RW(uint8_t, t1sz, 16, 6);
	BITFIELD_FIELD_RW(bool, a1, 22, 1);
	BITFIELD_FIELD_RW(bool, epd1, 23, 1);
	BITFIELD_FIELD_RW(uint8_t, irgn1, 24, 2);
	BITFIELD_FIELD_RW(uint8_t, orgn1, 26, 2);
	BITFIELD_FIELD_RW(uint8_t, sh1, 28, 2);
	BITFIELD_FIELD_RW(uint8_t, tg1, 30, 2);
	BITFIELD_FIELD_RW(memory::physical_address_size, ips, 32, 3);
	BITFIELD_FIELD_RW(bool, as, 36, 1);
	BITFIELD_FIELD_RW(bool, tbi0, 37, 1);
	BITFIELD_FIELD_RW(bool, tbi1, 38, 1);
	BITFIELD_FIELD_RW(bool, ha, 39, 1);
	BITFIELD_FIELD_RW(bool, hd, 40, 1);
	BITFIELD_FIELD_RW(bool, hpd0, 41, 1);
	BITFIELD_FIELD_RW(bool, hpd1, 42, 1);
	BITFIELD_FIELD_RW(bool, hwu059, 43, 1);
	BITFIELD_FIELD_RW(bool, hwu060, 44, 1);
	BITFIELD_FIELD_RW(bool, hwu061, 45, 1);
	BITFIELD_FIELD_RW(bool, hwu062, 46, 1);
	BITFIELD_FIELD_RW(bool, hwu159, 47, 1);
	BITFIELD_FIELD_RW(bool, hwu160, 48, 1);
	BITFIELD_FIELD_RW(bool, hwu161, 49, 1);
	BITFIELD_FIELD_RW(bool, hwu162, 50, 1);
	BITFIELD_FIELD_RW(bool, tbid0, 51, 1);
	BITFIELD_FIELD_RW(bool, tbid1, 52, 1);
	BITFIELD_FIELD_RW(bool, nfd0, 53, 1);
	BITFIELD_FIELD_RW(bool, nfd1, 54, 1);
	BITFIELD_FIELD_RW(bool, e0pd0, 55, 1);
	BITFIELD_FIELD_RW(bool, e0pd1, 56, 1);
	BITFIELD_FIELD_RW(bool, tcma0, 57, 1);
	BITFIELD_FIELD_RW(bool, tcma1, 58, 1);
	BITFIELD_FIELD_RW(bool, ds, 59, 1);
	BITFIELD_FIELD_RW(bool, mtx0, 60, 1);
	BITFIELD_FIELD_RW(bool, mtx1, 61, 1);
};

DEFINE_REGISTER_RW_FUNCS(tcr_el1);

REGISTER_CHECK(tcr_el1);

struct tcr_el2 {
	BITFIELD_DATA_UINT64(0x80800000);

	BITFIELD_FIELD_RW(uint8_t, t0sz, 0, 6);
	BITFIELD_FIELD_RW(memory::cacheability, inner_cacheability, 8, 2);
	BITFIELD_FIELD_RW(memory::cacheability, outer_cacheability, 10, 2);
	BITFIELD_FIELD_RW(memory::shareability, shareability, 12, 2);
	BITFIELD_FIELD_RW(paging::granule_size, granule_size, 14, 2);
	BITFIELD_FIELD_RW(memory::physical_address_size, physical_address_size, 16, 3);
	BITFIELD_FIELD_RW(bool, tbi, 20, 1);
	BITFIELD_FIELD_RW(bool, ha, 21, 1);
	BITFIELD_FIELD_RW(bool, hd, 22, 1);
	BITFIELD_FIELD_RW(bool, hpd, 24, 1);
	BITFIELD_FIELD_RW(bool, hwu59, 25, 1);
	BITFIELD_FIELD_RW(bool, hwu60, 26, 1);
	BITFIELD_FIELD_RW(bool, hwu61, 27, 1);
	BITFIELD_FIELD_RW(bool, hwu62, 28, 1);
	BITFIELD_FIELD_RW(bool, tbid, 29, 1);
	BITFIELD_FIELD_RW(bool, tcma, 30, 1);
	BITFIELD_FIELD_RW(bool, ds, 32, 1);
	BITFIELD_FIELD_RW(bool, mtx, 33, 1);
};

DEFINE_REGISTER_RW_FUNCS(tcr_el2);

REGISTER_CHECK(tcr_el2);

union indirect_mem_attr {
	BITFIELD_DATA_UINT64(0x0);
	memory::indirect_attribute attributes[8];
};

DEFINE_RENAME_REGISTER_RW_FUNCS(indirect_mem_attr, mair_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(indirect_mem_attr, mair_el1);

REGISTER_CHECK(indirect_mem_attr);

struct xlate_tbl_base {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(uintptr_t, base_address_upper, 1, 47);

	BITFIELD_FIELD_RW(bool, common_not_private, 0, 1);
	// Only valid for TTBRx_EL1 or E2H hosts
	BITFIELD_FIELD_RW(uint8_t, asid, 48, 16);

	constexpr xlate_tbl_base with_base_address(uintptr_t base_address) {
		return this->with_base_address_upper(base_address >> base_address_upper_SHIFT);
	}
};

DEFINE_RENAME_REGISTER_RW_FUNCS(xlate_tbl_base, ttbr0_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(xlate_tbl_base, ttbr0_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(xlate_tbl_base, ttbr1_el1);

REGISTER_CHECK(xlate_tbl_base);

DEFINE_DUMMY_REGISTER(par_el1);
REGISTER_CHECK(par_el1);

//
// Exceptions
//

union vector_base {
	BITFIELD_DATA_UINT64(0x0);
	/** Base address of the exception handler table
	 *
	 * Must be aligned to 2048 bytes
	 */
	uintptr_t address;
};

DEFINE_RENAME_REGISTER_RW_FUNCS(vector_base, vbar_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(vector_base, vbar_el1);

REGISTER_CHECK(vector_base);

struct saved_program_status {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(bool, is_aarch32, 4, 1);

	BITFIELD_FIELD_RW(bool, fiq_mask, 6, 1);
	BITFIELD_FIELD_RW(bool, irq_mask, 7, 1);
	BITFIELD_FIELD_RW(bool, serror_mask, 8, 1);

	BITFIELD_FIELD_RW(bool, illegal_execution_state, 20, 1);
	BITFIELD_FIELD_RW(bool, software_step, 21, 1);
	BITFIELD_FIELD_RW(bool, privileged_access_never, 22, 1);
	BITFIELD_FIELD_RW(bool, data_independent_timing, 24, 1);

	BITFIELD_FIELD_RW(bool, overflow, 28, 1);
	BITFIELD_FIELD_RW(bool, carry, 29, 1);
	BITFIELD_FIELD_RW(bool, zero, 30, 1);
	BITFIELD_FIELD_RW(bool, negative, 31, 1);

	BITFIELD_FIELD_RW(bool, pmu_exception_pending, 33, 1);

	BITFIELD_FIELD_RW(bool, use_sp_elx, 0, 1);
	BITFIELD_FIELD_RW(bool, must_be_zero, 1, 1);
	BITFIELD_FIELD_RW(exception_level, el, 2, 2);
	BITFIELD_FIELD_RW(bool, debug_mask, 9, 1);
	BITFIELD_FIELD_RW(uint8_t, branch_type_indicator, 10, 2);
	BITFIELD_FIELD_RW(bool, speculative_store_bypass, 12, 1);
	BITFIELD_FIELD_RW(bool, all_irq_fiq_mask, 13, 1);
	BITFIELD_FIELD_RW(bool, user_access_override, 23, 1);
	BITFIELD_FIELD_RW(bool, tag_check_override, 25, 1);
	BITFIELD_FIELD_RW(bool, pmu_mask, 32, 1);
	BITFIELD_FIELD_RW(bool, exception_return_state_lock, 34, 1);
	BITFIELD_FIELD_RW(bool, pacm, 35, 1);
};

REGISTER_CHECK(saved_program_status);

DEFINE_RENAME_REGISTER_RW_FUNCS(saved_program_status, spsr_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(saved_program_status, spsr_el1);

union exception_link {
	BITFIELD_DATA_UINT64(0x0);
	uintptr_t return_address;
};

DEFINE_RENAME_REGISTER_RW_FUNCS(exception_link, elr_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(exception_link, elr_el1);

REGISTER_CHECK(exception_link);

union exception_syndrome {
	BITFIELD_DATA_UINT64(0x0);

	// TODO More descriptive field accessors
	BITFIELD_FIELD_RW(uint32_t, iss, 0, 25);
	BITFIELD_FIELD_RW(bool, instruction_length_32bit, 25, 1);
	BITFIELD_FIELD_RW(enum exception_class, exception_class, 26, 6);
	BITFIELD_FIELD_RW(uint32_t, iss2, 32, 24);

	/** Fields shared for instruction and data aborts */
	struct mmu_abort_t {
		BITFIELD_DATA_UINT64(0x0);

		BITFIELD_FIELD_RW(mmu_abort_status_code, status_code, 0, 6);
		BITFIELD_FIELD_RW(bool, stage_1_ptw, 7, 1);
	} mmu_abort;

	struct data_abort {
		BITFIELD_DATA_UINT64(0x0);

		BITFIELD_FIELD_RW(bool, write_not_read, 6, 1);
		BITFIELD_FIELD_RW(bool, cache_maintenance, 8, 1);
		BITFIELD_FIELD_RW(bool, far_not_valid, 10, 1);
		BITFIELD_FIELD_RW(bool, vncr, 13, 1);
	} data_abort;

	struct eret {
		BITFIELD_DATA_UINT64(0x0);

		BITFIELD_FIELD_RW(bool, eretb, 0, 1);
		BITFIELD_FIELD_RW(bool, pauth_eret, 1, 1);
	} eret;

	struct system {
		BITFIELD_DATA_UINT64(0x0);

		BITFIELD_FIELD_RW(bool, is_read, 0, 1);
		BITFIELD_FIELD_RW(uint8_t, crm, 1, 4);
		BITFIELD_FIELD_RW(reg_enc, rt, 5, 5);
		BITFIELD_FIELD_RW(uint8_t, crn, 10, 4);
		BITFIELD_FIELD_RW(uint8_t, op1, 14, 3);
		BITFIELD_FIELD_RW(uint8_t, op2, 17, 3);
		BITFIELD_FIELD_RW(uint8_t, op0, 20, 2);
	} system;

	struct svc_hvc {
		BITFIELD_DATA_UINT64(0x0);

		BITFIELD_FIELD_RW(uint16_t, imm16, 0, 16);
	} svc_hvc;
};

DEFINE_RENAME_REGISTER_RW_FUNCS(exception_syndrome, esr_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(exception_syndrome, esr_el12);
DEFINE_RENAME_REGISTER_RW_FUNCS(exception_syndrome, esr_el1);


REGISTER_CHECK(exception_syndrome);

union faulting_address {
	BITFIELD_DATA_UINT64(0x0);

	uintptr_t faulting_address;
};

DEFINE_RENAME_REGISTER_RW_FUNCS(faulting_address, far_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(faulting_address, far_el12);
DEFINE_RENAME_REGISTER_RW_FUNCS(faulting_address, far_el1);


REGISTER_CHECK(faulting_address);

DEFINE_DUMMY_REGISTER(isr_el1);

//
// Timer
//

struct timer_control {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(bool, enable, 0, 1);
	BITFIELD_FIELD_RW(bool, interrupt_mask, 1, 1);
	BITFIELD_FIELD_RW(bool, status, 2, 1);
};

DEFINE_RENAME_REGISTER_RW_FUNCS(timer_control, cnthp_ctl_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(timer_control, cntv_ctl_el0);
DEFINE_RENAME_REGISTER_RW_FUNCS(timer_control, cntp_ctl_el0);

REGISTER_CHECK(timer_control);

/** Compare value for a timer */
union timer_cval {
	BITFIELD_DATA_UINT64(0x0);

	uint64_t compare_value;
};

DEFINE_RENAME_REGISTER_RW_FUNCS(timer_cval, cnthp_cval_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(timer_cval, cntv_cval_el0);
DEFINE_RENAME_REGISTER_RW_FUNCS(timer_cval, cntp_cval_el0);

REGISTER_CHECK(timer_cval);

union timer_tval {
	BITFIELD_DATA_UINT64(0x0);

	int64_t value;
};

DEFINE_RENAME_REGISTER_RW_FUNCS(timer_tval, cnthp_tval_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(timer_tval, cntv_tval_el0);
DEFINE_RENAME_REGISTER_RW_FUNCS(timer_tval, cntp_tval_el0);

REGISTER_CHECK(timer_tval);

//
// Hypervisor Control
//

struct hcr_el2 {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(bool, vm, 0, 1);
	BITFIELD_FIELD_RW(bool, swio, 1, 1);
	BITFIELD_FIELD_RW(bool, ptw, 2, 1);
	BITFIELD_FIELD_RW(bool, fmo, 3, 1);
	BITFIELD_FIELD_RW(bool, imo, 4, 1);
	BITFIELD_FIELD_RW(bool, amo, 5, 1);
	BITFIELD_FIELD_RW(bool, vf, 6, 1);
	BITFIELD_FIELD_RW(bool, vi, 7, 1);
	BITFIELD_FIELD_RW(bool, vse, 8, 1);
	BITFIELD_FIELD_RW(bool, fb, 9, 1);
	BITFIELD_FIELD_RW(uint8_t, bsu, 10, 2);
	BITFIELD_FIELD_RW(bool, dc, 12, 1);
	BITFIELD_FIELD_RW(bool, twi, 13, 1);
	BITFIELD_FIELD_RW(bool, twe, 14, 1);
	BITFIELD_FIELD_RW(bool, tid0, 15, 1);
	BITFIELD_FIELD_RW(bool, tid1, 16, 1);
	BITFIELD_FIELD_RW(bool, tid2, 17, 1);
	BITFIELD_FIELD_RW(bool, tid3, 18, 1);
	BITFIELD_FIELD_RW(bool, tsc, 19, 1);
	BITFIELD_FIELD_RW(bool, tidcp, 20, 1);
	BITFIELD_FIELD_RW(bool, tacr, 21, 1);
	BITFIELD_FIELD_RW(bool, tsw, 22, 1);
	BITFIELD_FIELD_RW(bool, tpcp, 23, 1);
	BITFIELD_FIELD_RW(bool, tpu, 24, 1);
	BITFIELD_FIELD_RW(bool, ttlb, 25, 1);
	BITFIELD_FIELD_RW(bool, tvm, 26, 1);
	BITFIELD_FIELD_RW(bool, tge, 27, 1);
	BITFIELD_FIELD_RW(bool, tdz, 28, 1);
	BITFIELD_FIELD_RW(bool, hcd, 29, 1);
	BITFIELD_FIELD_RW(bool, trvm, 30, 1);
	BITFIELD_FIELD_RW(bool, rw, 31, 1);
	BITFIELD_FIELD_RW(bool, cd, 32, 1);
	BITFIELD_FIELD_RW(bool, id, 33, 1);
	BITFIELD_FIELD_RW(bool, e2h, 34, 1);
	BITFIELD_FIELD_RW(bool, tlor, 35, 1);
	BITFIELD_FIELD_RW(bool, terr, 36, 1);
	BITFIELD_FIELD_RW(bool, tea, 37, 1);
	BITFIELD_FIELD_RW(bool, miocnce, 38, 1);
	BITFIELD_FIELD_RW(bool, tme, 39, 1);
	BITFIELD_FIELD_RW(bool, apk, 40, 1);
	BITFIELD_FIELD_RW(bool, api, 41, 1);
	BITFIELD_FIELD_RW(bool, nv, 42, 1);
	BITFIELD_FIELD_RW(bool, nv1, 43, 1);
	BITFIELD_FIELD_RW(bool, at, 44, 1);
	BITFIELD_FIELD_RW(bool, nv2, 45, 1);
	BITFIELD_FIELD_RW(bool, fwb, 46, 1);
	BITFIELD_FIELD_RW(bool, fien, 47, 1);
	BITFIELD_FIELD_RW(bool, gpf, 48, 1);
	BITFIELD_FIELD_RW(bool, tid4, 49, 1);
	BITFIELD_FIELD_RW(bool, ticab, 50, 1);
	BITFIELD_FIELD_RW(bool, amvoffen, 51, 1);
	BITFIELD_FIELD_RW(bool, tocu, 52, 1);
	BITFIELD_FIELD_RW(bool, enscxt, 53, 1);
	BITFIELD_FIELD_RW(bool, ttlbis, 54, 1);
	BITFIELD_FIELD_RW(bool, ttlbos, 55, 1);
	BITFIELD_FIELD_RW(bool, ata, 56, 1);
	BITFIELD_FIELD_RW(bool, dct, 57, 1);
	BITFIELD_FIELD_RW(bool, tid5, 58, 1);
	BITFIELD_FIELD_RW(bool, tweden, 59, 1);
	BITFIELD_FIELD_RW(uint8_t, twedel, 60, 4);
};

DEFINE_REGISTER_RW_FUNCS(hcr_el2);

REGISTER_CHECK(hcr_el2);

union cnthctl_el2 {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(bool, evnten, 2, 1);
	BITFIELD_FIELD_RW(bool, evntdir, 3, 1);
	BITFIELD_FIELD_RW(uint8_t, evnti, 4, 4);
	BITFIELD_FIELD_RW(bool, ecv, 12, 1);
	BITFIELD_FIELD_RW(bool, el1tvt, 13, 1);
	BITFIELD_FIELD_RW(bool, el1tvct, 14, 1);
	BITFIELD_FIELD_RW(bool, el1nvpct, 15, 1);
	BITFIELD_FIELD_RW(bool, el1nvvct, 16, 1);
	BITFIELD_FIELD_RW(bool, evntis, 17, 1);
	BITFIELD_FIELD_RW(bool, cntvmask, 18, 1);
	BITFIELD_FIELD_RW(bool, cntpmask, 19, 1);

	struct e2h {
		BITFIELD_DATA_UINT64(0x0);

		BITFIELD_FIELD_RW(bool, el0pcten, 0, 1);
		BITFIELD_FIELD_RW(bool, el0vcten, 1, 1);
		BITFIELD_FIELD_RW(bool, el0vten, 8, 1);
		BITFIELD_FIELD_RW(bool, el0pten, 9, 1);
		BITFIELD_FIELD_RW(bool, el1pcten, 10, 1);
		BITFIELD_FIELD_RW(bool, el1pten, 11, 1);
	} e2h;

	struct ne2h {
		BITFIELD_DATA_UINT64(0x0);

		BITFIELD_FIELD_RW(bool, el1pcten, 0, 1);
		BITFIELD_FIELD_RW(bool, el1pcen, 1, 1);
	} ne2h;
};

DEFINE_REGISTER_RW_FUNCS(cnthctl_el2);

REGISTER_CHECK(cnthctl_el2);

DEFINE_DUMMY_REGISTER(cntvoff_el2);

struct hpfar_el2 {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RO(uintptr_t, fipa, 4, 40);
	BITFIELD_FIELD_RO(bool, ns, 63, 1);
};

DEFINE_REGISTER_RW_FUNCS(hpfar_el2);

REGISTER_CHECK(hpfar_el2);

struct hstr_el2 {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(bool, t0, 0, 1);
	BITFIELD_FIELD_RW(bool, t1, 1, 1);
	BITFIELD_FIELD_RW(bool, t2, 2, 1);
	BITFIELD_FIELD_RW(bool, t3, 3, 1);
	BITFIELD_FIELD_RW(bool, t5, 5, 1);
	BITFIELD_FIELD_RW(bool, t6, 6, 1);
	BITFIELD_FIELD_RW(bool, t7, 7, 1);
	BITFIELD_FIELD_RW(bool, t8, 8, 1);
	BITFIELD_FIELD_RW(bool, t9, 9, 1);
	BITFIELD_FIELD_RW(bool, t10, 10, 1);
	BITFIELD_FIELD_RW(bool, t11, 11, 1);
	BITFIELD_FIELD_RW(bool, t12, 12, 1);
	BITFIELD_FIELD_RW(bool, t13, 13, 1);
	BITFIELD_FIELD_RW(bool, t15, 15, 1);
};

DEFINE_REGISTER_RW_FUNCS(hstr_el2);

REGISTER_CHECK(hstr_el2);

struct mpid {
	BITFIELD_DATA_UINT64(0xC0000000);

	BITFIELD_FIELD_RW(uint8_t, aff0, 0, 8);
	BITFIELD_FIELD_RW(uint8_t, aff1, 8, 8);
	BITFIELD_FIELD_RW(uint8_t, aff2, 16, 8);
	BITFIELD_FIELD_RW(bool, multithreading, 24, 1);
	BITFIELD_FIELD_RW(bool, uniprocessor, 30, 1);
	BITFIELD_FIELD_RW(uint8_t, aff3, 32, 8);
};

DEFINE_RENAME_REGISTER_RW_FUNCS(mpid, vmpidr_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(mpid, mpidr_el1);

REGISTER_CHECK(mpid);

struct pid {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(uint8_t, revision, 0, 4);
	BITFIELD_FIELD_RW(uint16_t, partnum, 4, 12);
	BITFIELD_FIELD_RW(uint8_t, architecture, 16, 4);
	BITFIELD_FIELD_RW(uint8_t, variant, 20, 4);
	BITFIELD_FIELD_RW(uint8_t, implementer, 24, 8);
};

DEFINE_RENAME_REGISTER_RW_FUNCS(pid, vpidr_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(pid, midr_el1);

REGISTER_CHECK(pid);

struct vtcr_el2 {
	BITFIELD_DATA_UINT64(0xC0000000);

	BITFIELD_FIELD_RW(uint8_t, t0sz, 0, 6);
	BITFIELD_FIELD_RW(paging::stage_2_starting_level, sl0, 6, 2);
	BITFIELD_FIELD_RW(memory::cacheability, inner_cacheability, 8, 2);
	BITFIELD_FIELD_RW(memory::cacheability, outer_cacheability, 10, 2);
	BITFIELD_FIELD_RW(memory::shareability, shareability, 12, 2);
	BITFIELD_FIELD_RW(paging::granule_size, granule_size, 14, 2);
	BITFIELD_FIELD_RW(memory::physical_address_size, physical_address_size, 16, 3);
	BITFIELD_FIELD_RW(bool, large_vmid, 19, 1);
	BITFIELD_FIELD_RW(bool, hw_access, 21, 1);
	BITFIELD_FIELD_RW(bool, hw_dirty, 22, 1);
	BITFIELD_FIELD_RW(bool, hwu59, 25, 1);
	BITFIELD_FIELD_RW(bool, hwu60, 26, 1);
	BITFIELD_FIELD_RW(bool, hwu61, 27, 1);
	BITFIELD_FIELD_RW(bool, hwu62, 28, 1);
	BITFIELD_FIELD_RW(bool, nsw, 29, 1);
	BITFIELD_FIELD_RW(bool, nsa, 30, 1);
	BITFIELD_FIELD_RW(bool, ds, 32, 1);
	BITFIELD_FIELD_RW(bool, sl2, 33, 1);
	BITFIELD_FIELD_RW(bool, assured_only, 34, 1);
	BITFIELD_FIELD_RW(bool, tl1, 35, 1);
	BITFIELD_FIELD_RW(bool, s2pie, 36, 1);
	BITFIELD_FIELD_RW(bool, s2poe, 37, 1);
	BITFIELD_FIELD_RW(bool, d128, 38, 1);
	BITFIELD_FIELD_RW(bool, gcsh, 40, 1);
	BITFIELD_FIELD_RW(bool, tl0, 41, 1);
	BITFIELD_FIELD_RW(bool, haft, 44, 1);
	BITFIELD_FIELD_RW(bool, hdbss, 45, 1);
};

DEFINE_REGISTER_RW_FUNCS(vtcr_el2);

REGISTER_CHECK(vtcr_el2);

struct vttbr_el2 {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(uint64_t, base_address_upper, 1, 47);

	BITFIELD_FIELD_RW(bool, common_not_private, 0, 1);
	BITFIELD_FIELD_RW(uint16_t, vmid, 48, 16);

	constexpr uintptr_t base_address() const {
		return (base_address_upper() << base_address_upper_SHIFT);
	}

	constexpr void set_base_address(uintptr_t base_address) {
		set_base_address_upper(base_address >> base_address_upper_SHIFT);
	}

	constexpr vttbr_el2 with_base_address(uintptr_t base_address) const {
		auto copy = *this;
		copy.set_base_address(base_address);
		return copy;
	}
};

DEFINE_REGISTER_RW_FUNCS(vttbr_el2);

REGISTER_CHECK(vttbr_el2);

union cptr_el2 {
	BITFIELD_DATA_UINT64(0x0);

	struct e2h {
		BITFIELD_DATA_UINT64(0x0);

		BITFIELD_FIELD_RW(fp_trap, zen, 16, 2);
		BITFIELD_FIELD_RW(fp_trap, fpen, 20, 2);
		BITFIELD_FIELD_RW(fp_trap, smen, 24, 2);
		BITFIELD_FIELD_RW(bool, tta, 28, 1);
		BITFIELD_FIELD_RW(bool, e0poe, 29, 1);
		BITFIELD_FIELD_RW(bool, tam, 30, 1);
		BITFIELD_FIELD_RW(bool, tcpac, 31, 1);
	} e2h;

	static constexpr cptr_el2 for_e2h() {
		return (cptr_el2 { .e2h = {} });
	}

	struct ne2h {
		BITFIELD_DATA_UINT64(0x23FF);

		BITFIELD_FIELD_RW(bool, tz, 8, 1);
		BITFIELD_FIELD_RW(bool, tfp, 10, 1);
		BITFIELD_FIELD_RW(bool, tsm, 12, 1);
		BITFIELD_FIELD_RW(bool, tta, 20, 1);
		BITFIELD_FIELD_RW(bool, tam, 30, 1);
		BITFIELD_FIELD_RW(bool, tcpac, 31, 1);
	} ne2h;

	static constexpr cptr_el2 for_ne2h() {
		return (cptr_el2 { .ne2h = {} });
	}
};

DEFINE_REGISTER_RW_FUNCS(cptr_el2);

REGISTER_CHECK(cptr_el2);

//
// Auxiliary Registers
//

DEFINE_DUMMY_REGISTER(amair_el2);
DEFINE_DUMMY_REGISTER(actlr_el2);
DEFINE_DUMMY_REGISTER(afsr0_el2);
DEFINE_DUMMY_REGISTER(afsr1_el2);
DEFINE_DUMMY_REGISTER(hacr_el2);

DEFINE_DUMMY_REGISTER(actlr_el1);
DEFINE_DUMMY_REGISTER(afsr0_el1);
DEFINE_DUMMY_REGISTER(afsr1_el1);
DEFINE_DUMMY_REGISTER(amair_el1);

//
// Debug
//

struct mdcr_el2 {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(uint8_t, hpmn, 0, 5);
	BITFIELD_FIELD_RW(bool, tpmcr, 5, 1);
	BITFIELD_FIELD_RW(bool, tpm, 6, 1);
	BITFIELD_FIELD_RW(bool, hpme, 7, 1);
	BITFIELD_FIELD_RW(bool, tde, 8, 1);
	BITFIELD_FIELD_RW(bool, tda, 9, 1);
	BITFIELD_FIELD_RW(bool, tdosa, 10, 1);
	BITFIELD_FIELD_RW(bool, tdra, 11, 1);
	BITFIELD_FIELD_RW(uint8_t, e2pb, 12, 2);
	BITFIELD_FIELD_RW(bool, tpms, 14, 1);
	BITFIELD_FIELD_RW(bool, enspm, 15, 1);
	BITFIELD_FIELD_RW(bool, hpmd, 17, 1);
	BITFIELD_FIELD_RW(bool, ttrf, 19, 1);
	BITFIELD_FIELD_RW(bool, hccd, 23, 1);
	BITFIELD_FIELD_RW(uint8_t, e2tb, 24, 2);
	BITFIELD_FIELD_RW(bool, hlp, 26, 1);
	BITFIELD_FIELD_RW(bool, tdcc, 27, 1);
	BITFIELD_FIELD_RW(bool, mtpme, 28, 1);
	BITFIELD_FIELD_RW(bool, hpmfzo, 29, 1);
	BITFIELD_FIELD_RW(uint8_t, pmsse, 30, 2);
	BITFIELD_FIELD_RW(bool, hpmfzs, 36, 1);
	BITFIELD_FIELD_RW(uint8_t, pmee, 40, 2);
	BITFIELD_FIELD_RW(bool, ebwe, 43, 1);
	BITFIELD_FIELD_RW(bool, enstepop, 50, 1);
};

DEFINE_REGISTER_RW_FUNCS(mdcr_el2);

REGISTER_CHECK(mdcr_el2);

// DEFINE_DUMMY_REGISTER(mdscr_el1);

struct mdscr_el1 {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(bool, ss, 0, 1);
  	BITFIELD_FIELD_RW(bool, err, 6, 1);
	BITFIELD_FIELD_RW(bool, tdcc, 12, 1);
	BITFIELD_FIELD_RW(bool, kde, 13, 1);
	BITFIELD_FIELD_RW(bool, hde, 14, 1);
	BITFIELD_FIELD_RW(bool, mde, 15, 1);
	BITFIELD_FIELD_RW(bool, sc2, 19, 1);
	BITFIELD_FIELD_RW(bool, tda, 21, 1);
    BITFIELD_FIELD_RW(uint8_t, intdis, 22, 2);
	BITFIELD_FIELD_RW(bool, txu, 26, 1);
	BITFIELD_FIELD_RW(bool, rxo, 27, 1);
	BITFIELD_FIELD_RW(bool, txfull, 29, 1);
	BITFIELD_FIELD_RW(bool, rxfull, 30, 1);
	BITFIELD_FIELD_RW(bool, tfo, 31, 1);
	BITFIELD_FIELD_RW(bool, embwe, 32, 1);
	BITFIELD_FIELD_RW(bool, tta, 33, 1);
	BITFIELD_FIELD_RW(bool, enspm, 34, 1);
	BITFIELD_FIELD_RW(bool, ehbwe, 35, 1);
};

DEFINE_REGISTER_RW_FUNCS(mdscr_el1);

REGISTER_CHECK(mdscr_el1);


struct dbgbvr_n_el1 {
  BITFIELD_DATA_UINT64(0x0);
};

REGISTER_CHECK(dbgbvr_n_el1);

DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbvr_n_el1, dbgbvr0_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbvr_n_el1, dbgbvr1_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbvr_n_el1, dbgbvr2_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbvr_n_el1, dbgbvr3_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbvr_n_el1, dbgbvr4_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbvr_n_el1, dbgbvr5_el1);

struct dbgbcr_n_el1 {
  BITFIELD_DATA_UINT64(0x0);

  BITFIELD_FIELD_RW(bool, e, 0, 1);
  BITFIELD_FIELD_RW(uint8_t, pmc, 1, 2);
  BITFIELD_FIELD_RW(bool, bt2, 3, 1);
  BITFIELD_FIELD_RW(uint8_t, bas, 5, 4);
  BITFIELD_FIELD_RW(bool, hmc, 13, 1);
  BITFIELD_FIELD_RW(uint8_t, ssc, 14, 2);
  BITFIELD_FIELD_RW(uint8_t, lbn, 16, 4);
  BITFIELD_FIELD_RW(uint8_t, bt, 20, 4);
  BITFIELD_FIELD_RW(uint8_t, mask, 24, 5);
  BITFIELD_FIELD_RW(bool, ssce, 29, 1);
  BITFIELD_FIELD_RW(uint8_t, lbnx, 30, 2);
};

REGISTER_CHECK(dbgbcr_n_el1);

DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbcr_n_el1, dbgbcr0_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbcr_n_el1, dbgbcr1_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbcr_n_el1, dbgbcr2_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbcr_n_el1, dbgbcr3_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbcr_n_el1, dbgbcr4_el1);
DEFINE_RENAME_REGISTER_RW_FUNCS(dbgbcr_n_el1, dbgbcr5_el1);

//
// Various EL1 Registers
//

struct cntkctl_el1 {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(bool, el0pcten, 0, 1);
	BITFIELD_FIELD_RW(bool, el0vcten, 1, 1);
	BITFIELD_FIELD_RW(bool, evnten, 2, 1);
	BITFIELD_FIELD_RW(bool, evntdir, 3, 1);
	BITFIELD_FIELD_RW(uint8_t, evnti, 4, 4);
	BITFIELD_FIELD_RW(bool, el0vten, 8, 1);
	BITFIELD_FIELD_RW(bool, el0pten, 9, 1);
	BITFIELD_FIELD_RW(bool, evntis, 17, 1);
};

DEFINE_REGISTER_RW_FUNCS(cntkctl_el1);

REGISTER_CHECK(cntkctl_el1);

//
// FEAT_TCR2
//

DEFINE_DUMMY_REGISTER(tcr2_el1);

//
// FEAT_SCTLR2
//

DEFINE_DUMMY_REGISTER(sctlr2_el1);

//
// FEAT_SPE
//

DEFINE_DUMMY_REGISTER(pmblimitr_el1);
DEFINE_DUMMY_REGISTER(pmbptr_el1);
DEFINE_DUMMY_REGISTER(pmbsr_el1);
DEFINE_DUMMY_REGISTER(pmscr_el1);
DEFINE_DUMMY_REGISTER(pmsevfr_el1);
DEFINE_DUMMY_REGISTER(pmsicr_el1);
DEFINE_DUMMY_REGISTER(pmsirr_el1);
DEFINE_DUMMY_REGISTER(pmslatfr_el1);
DEFINE_DUMMY_REGISTER(pmsnevfr_el1);

//
// FEAT_TRF
//

DEFINE_DUMMY_REGISTER(trfcr_el1);

//
// FEAT_BRBE
//

DEFINE_DUMMY_REGISTER(brbcr_el1);

//
// FEAT_MPAM
//

DEFINE_DUMMY_REGISTER(mpam1_el1);
DEFINE_DUMMY_REGISTER(mpamvpmv_el2);
DEFINE_DUMMY_REGISTER(mpamhcr_el2);

struct mpamvpm_n_el2 {
	BITFIELD_DATA_UINT64(0x0);
};

REGISTER_CHECK(mpamvpm_n_el2);

DEFINE_RENAME_REGISTER_RW_FUNCS(mpamvpm_n_el2, mpamvpm0_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(mpamvpm_n_el2, mpamvpm1_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(mpamvpm_n_el2, mpamvpm2_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(mpamvpm_n_el2, mpamvpm3_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(mpamvpm_n_el2, mpamvpm4_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(mpamvpm_n_el2, mpamvpm5_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(mpamvpm_n_el2, mpamvpm6_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(mpamvpm_n_el2, mpamvpm7_el2);

//
// FEAT_CSV2_2
//

DEFINE_DUMMY_REGISTER(scxtnum_el1);

//
// FEAT_MTE2
//

DEFINE_DUMMY_REGISTER(tfsr_el1);

//
// FEAT_SVE
//

DEFINE_DUMMY_REGISTER(zcr_el1);

//
// FEAT_SME
//

DEFINE_DUMMY_REGISTER(smcr_el1);
DEFINE_DUMMY_REGISTER(smprimap_el2);

//
// FEAT_NV2
//

struct vncr_el2 {
	BITFIELD_DATA_UINT64(0x0);

	BITFIELD_FIELD_RW(uintptr_t, base_address_upper, 12, 52);

	vncr_el2 with_base_address(uintptr_t base_address) {
		assert(is_aligned(base_address, 0x1000));
		return this->with_base_address_upper(base_address >> base_address_upper_SHIFT);
	}
};

DEFINE_REGISTER_RW_FUNCS(vncr_el2);

REGISTER_CHECK(vncr_el2);

//
// FEAT_VHE
//

DEFINE_RENAME_REGISTER_RW_FUNCS(ctx_id, contextidr_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(xlate_tbl_base, ttbr1_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(timer_control, cnthv_ctl_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(timer_cval, cnthv_cval_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(timer_tval, cnthv_tval_el2);

// GIC

DEFINE_DUMMY_REGISTER(ich_vtr_el2);
DEFINE_DUMMY_REGISTER(icc_sre_el2);
DEFINE_DUMMY_REGISTER(ich_elrsr_el2);
DEFINE_DUMMY_REGISTER(ich_hcr_el2);
DEFINE_DUMMY_REGISTER(ich_vmcr_el2);

struct ich_lr_n_el2 {
	BITFIELD_DATA_UINT64(0x0);
};

REGISTER_CHECK(ich_lr_n_el2);

DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr0_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr1_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr2_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr3_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr4_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr5_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr6_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr7_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr8_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr9_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr10_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr11_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr12_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr13_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr14_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_lr_n_el2, ich_lr15_el2);

struct ich_ap0r_n_el2 {
	BITFIELD_DATA_UINT64(0x0);
};

REGISTER_CHECK(ich_ap0r_n_el2);

DEFINE_RENAME_REGISTER_RW_FUNCS(ich_ap0r_n_el2, ich_ap0r0_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_ap0r_n_el2, ich_ap0r1_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_ap0r_n_el2, ich_ap0r2_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_ap0r_n_el2, ich_ap0r3_el2);

struct ich_ap1r_n_el2 {
	BITFIELD_DATA_UINT64(0x0);
};

REGISTER_CHECK(ich_ap1r_n_el2);

DEFINE_RENAME_REGISTER_RW_FUNCS(ich_ap1r_n_el2, ich_ap1r0_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_ap1r_n_el2, ich_ap1r1_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_ap1r_n_el2, ich_ap1r2_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(ich_ap1r_n_el2, ich_ap1r3_el2);

//
// FEAT_HCX
//

DEFINE_DUMMY_REGISTER(hcrx_el2);

//
// FEAT_ECV
//

DEFINE_DUMMY_REGISTER(cntpoff_el2);
DEFINE_DUMMY_REGISTER(cntpctss_el0);

//
// FEAT_FGT
//

DEFINE_DUMMY_REGISTER(hfgrtr_el2);
DEFINE_DUMMY_REGISTER(hfgwtr_el2);
DEFINE_DUMMY_REGISTER(hfgitr_el2);
DEFINE_DUMMY_REGISTER(hdfgrtr_el2);
DEFINE_DUMMY_REGISTER(hdfgwtr_el2);

//
// FEAT_FGT && FEAT_AMUv1
//

DEFINE_DUMMY_REGISTER(hafgrtr_el2);

//
// FEAT_RAS
//

DEFINE_DUMMY_REGISTER(vdisr_el2);
DEFINE_DUMMY_REGISTER(vsesr_el2);

//
// FEAT_AMUv1p1
//

struct amevcntvoff0_n_el2 {
	BITFIELD_DATA_UINT64(0x0);
};

REGISTER_CHECK(amevcntvoff0_n_el2);

DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff00_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff01_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff02_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff03_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff04_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff05_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff06_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff07_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff08_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff09_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff010_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff011_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff012_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff013_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff014_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff0_n_el2, amevcntvoff015_el2);

struct amevcntvoff1_n_el2 {
	BITFIELD_DATA_UINT64(0x0);
};

REGISTER_CHECK(amevcntvoff1_n_el2);

DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff10_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff11_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff12_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff13_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff14_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff15_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff16_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff17_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff18_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff19_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff110_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff111_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff112_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff113_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff114_el2);
DEFINE_RENAME_REGISTER_RW_FUNCS(amevcntvoff1_n_el2, amevcntvoff115_el2);
