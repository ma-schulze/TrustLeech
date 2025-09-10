#include "dbg/vmi.hpp"
#include "drivers/network.hpp"
#include "drivers/virtio-drivers.h"
#include "log.hpp"
#include "paging/shadow/table_walk.hpp"
#include "tail.hpp"
#include <types.hpp>
#include <arm-trusted-firmware/common/context_stash_exp.h>

#include <arch.hpp>
#include <context_stash.hpp>
#include <exception/head.hpp>
#include <exec_vel2.hpp>
#include <exec_vm.hpp>
#include <paging/stage_1.hpp>
#include <paging/vpas.hpp>
#include <plat/feat.hpp>
#include <plat/paging.hpp>
#include <registers.hpp>
#include <sysops.hpp>
#include <vcpu_ctx.hpp>

#include <lib/libc/stdio.hpp>
#include <dbg/dbg.hpp>

namespace
{
/** Context controlling the MMU (see I_TNCZR)
	 *
	 * This excludes SCTLR_EL2 as that was already saved by the firmware as part of
	 * the context stash
	 */
struct mmu_ctx {
	struct tcr_el2 tcr_el2;
	union indirect_mem_attr mair_el2;
	struct amair_el2 amair_el2;
	struct xlate_tbl_base ttbr0_el2;

	/** Retrieve the old MMU context we need to restore once we dispatch the
	 	 * actual system again
	 	 */
	static mmu_ctx save()
	{
		return mmu_ctx{
			read_tcr_el2(),
			read_mair_el2(),
			read_amair_el2(),
			read_ttbr0_el2(),
		};
	}

	/** Get some initial context for usage in the hypervisor */
	static mmu_ctx get_initial()
	{
		constexpr struct tcr_el2 INIT_TCR_EL2 =
			(struct tcr_el2){}
				.with_t0sz(plat::stage_1::address_space::SZ)
				/* We do not write our translation tables after setup, so no sense
			 	 * in write caching or sharing
	 	 	 	 */
				.with_inner_cacheability(
					memory::cacheability::
						write_back_read_allocate_no_write_allocate)
				.with_outer_cacheability(
					memory::cacheability::
						write_back_read_allocate_no_write_allocate)
				.with_shareability(memory::shareability::none)
				.with_granule_size(paging::granule_size::G64KiB)
				/* TODO(platform) This value is platform dependent on
				 * ID_AA64MMFR0_EL1.PARange
				 */
				.with_physical_address_size(
					memory::physical_address_size::
						PAS_48_BITS);

		constexpr union indirect_mem_attr INIT_MAIR_EL2{ .attributes{
			memory::indirect_attribute::
				normal_non_transient_always_write_back_always_allocate,
		} };

		/* TODO(platform) AMAIR_EL2 is platform specific, however its
			 * unused in QEMU. When a platform actually using it is
			 * implemented, we would need to provide a platform specific
			 * initial value here
	 	 	 */
		constexpr struct amair_el2 INIT_AMAIR_EL2;

		const struct xlate_tbl_base INIT_TTBR0_EL2 = []() {
			struct xlate_tbl_base init_ttbr0_el2;

			if constexpr (plat::supports(feat::ttcnp)) {
				init_ttbr0_el2.set_common_not_private(true);
			}

			uintptr_t page_table = paging::stage_1::get_table();

			return init_ttbr0_el2.with_base_address(page_table);
		}();

		return mmu_ctx{
			INIT_TCR_EL2,
			INIT_MAIR_EL2,
			INIT_AMAIR_EL2,
			INIT_TTBR0_EL2,
		};
	}

	void write() const
	{
		write_tcr_el2(tcr_el2);
		write_mair_el2(mair_el2);
		write_amair_el2(amair_el2);
		write_ttbr0_el2(ttbr0_el2);
	}
};

constexpr sctlr INIT_SCTLR_EL2 = sctlr::for_el2()
					 .with_mmu(true)
					 .with_alignment_check(false)
					 .with_data_cacheability(true)
					 .with_sp_alignment_check(false)
					 .with_instruction_cacheability(true)
					 .with_write_execute_never(true)
					 .with_big_endian(false);

mmu_ctx setup_mmu()
{
	auto saved_ctx = mmu_ctx::save();

	mmu_ctx::get_initial().write();

	/* Ensure the control register changes are seen before enabling the MMU */
	isb();

	write_sctlr_el2(INIT_SCTLR_EL2);

	isb();

	return saved_ctx;
}

vector_base setup_exception_handlers()
{
	auto saved_ctx = read_vbar_el2();

	vector_base init_vbar_el2;
	init_vbar_el2.address = reinterpret_cast<uintptr_t>(&exception_vectors);
	write_vbar_el2(init_vbar_el2);

	return saved_ctx;
}

constexpr hcr_el2 INIT_HCR_EL2{};

hcr_el2 setup_dummy_hcr_el2()
{
	auto saved_ctx = read_hcr_el2();

	write_hcr_el2(INIT_HCR_EL2);

	return saved_ctx;
}
}

void init_drivers(void)
{
}

void init_console(void)
{
	struct console_list *csl_list;
	struct console_info *console_ptr;
	uintptr_t uart_base;
	unsigned int uart_clk, uart_baud;
	device_enumerate(0x40000000, 0, 0, 0);

	setup_network();
	LOG_INFO("setup network!");

	/* RMM currently only supports one console */
	// int ret = pl011_init(PLAT_QEMU_BOOT_UART_BASE,
	// 		     PLAT_QEMU_BOOT_UART_CLK_IN_HZ,
	// 		     PLAT_QEMU_CONSOLE_BAUDRATE);
	// if (ret != 0) {
	// 	printf("Could not init the console driver!\n");
	// }

	// printf("We actually print stuff with the printi boy :)\n");
}

static volatile uint8_t setup_lock = 0x0;
static volatile uint8_t driver_lock = 0;

extern "C" void setup_cpu(void)
{
	auto this_core = []() {
		uint64_t v = read_mpidr_el1().v;
		v &= static_cast<uint64_t>(0xff);
		return v;
	}();

	/* Flush out remnants of an existing system operating at EL2 */
	tlbi_alle2();

	auto vbar_el2 = setup_exception_handlers();
	auto hcr_el2 = setup_dummy_hcr_el2();

	// TODO(muticore) Only on the first core, others must wait for barrier

	if (this_core == 0) {
		paging::stage_1::setup();
		setup_lock = 1;
	} else {
		while (!setup_lock) {
		}
	}

	auto mmu_ctx = setup_mmu();

	auto tpidr_el2 = setup_tpidr_el2();

	paging::vpas::setup();
	;

	vcpu().set_sctlr_el2(sctlr{ (&context_stash)[this_core].sctlr_el2 });
	vcpu().set_sp_el2(stack_pointer{ (&context_stash)[this_core].sp_el2 });
	vcpu().set_tpidr_el2(tpidr_el2);

	vcpu().set_vbar_el2(vbar_el2);
	vcpu().set_spsr_el2(read_spsr_el2());
	vcpu().set_elr_el2(read_elr_el2());
	vcpu().set_esr_el2(read_esr_el2());
	vcpu().set_far_el2(read_far_el2());

	vcpu().set_tcr_el2(mmu_ctx.tcr_el2);
	vcpu().set_mair_el2(mmu_ctx.mair_el2);
	vcpu().set_ttbr0_el2(mmu_ctx.ttbr0_el2);

	vcpu().set_hcr_el2(hcr_el2);
	vcpu().set_cntvoff_el2(read_cntvoff_el2());
	vcpu().last_cntvoff = vcpu().cntvoff_el2().v;
	vcpu().set_cnthctl_el2(read_cnthctl_el2());
	vcpu().set_hpfar_el2(read_hpfar_el2());
	vcpu().set_hstr_el2(read_hstr_el2());
	vcpu().set_vmpidr_el2(read_vmpidr_el2());
	vcpu().set_vpidr_el2(read_vpidr_el2());
	vcpu().set_vtcr_el2(read_vtcr_el2());
	vcpu().set_vttbr_el2(read_vttbr_el2());
	vcpu().set_cptr_el2(read_cptr_el2());

	vcpu().set_amair_el2(mmu_ctx.amair_el2);
	vcpu().set_actlr_el2(read_actlr_el2());
	vcpu().set_afsr0_el2(read_afsr0_el2());
	vcpu().set_afsr1_el2(read_afsr1_el2());
	vcpu().set_hacr_el2(read_hacr_el2());

	vcpu().set_mdcr_el2(read_mdcr_el2());

	static_assert(plat::supports(feat::nv2),
		      "FEAT_NV2 is required for TrustLeech");
	vcpu().set_vncr_el2(read_vncr_el2());

	write_hfgrtr_el2({ 1 << 18 }); // Trap ISR_EL1

	if (plat::supports(feat::vhe)) {
		vcpu().set_contextidr_el2(read_contextidr_el2());
		vcpu().set_ttbr1_el2(read_ttbr1_el2());
	}

	if constexpr (plat::supports(feat::hcx)) {
		vcpu().set_hcrx_el2({ read_hcrx_el2().v | 0x800 });
		write_hcrx_el2({ read_hcrx_el2().v | 0x800 });
	}

	if constexpr (plat::supports(feat::fgt)) {
		vcpu().set_hfgrtr_el2(read_hfgrtr_el2());
		vcpu().set_hfgwtr_el2(read_hfgwtr_el2());
		vcpu().set_hfgitr_el2(read_hfgitr_el2());
		vcpu().set_hdfgrtr_el2(read_hdfgrtr_el2());
		vcpu().set_hdfgwtr_el2(read_hdfgwtr_el2());
	}

	vcpu().save_program_state(
		saved_program_status{ (&context_stash)[this_core].spsr_el3 },
		(&context_stash)[this_core].elr_el3);

	vcpu().dispatch_vel2 =
		vcpu().cpsr.el() == EL2 ||
		(vcpu().cpsr.el() == EL0 && vcpu().hcr_el2().tge());

	// We run vEL2 in EL1
	if (vcpu().cpsr.el() == EL2) {
		vcpu().cpsr.set_el(EL1);
	}

	// choose Core 0 as boot core to init the drivers
	if (this_core == 0x0) {
		init_drivers();
		init_console();
		driver_lock = 1;
	} else {
		while (!driver_lock) {
		}
	}

	dbg::init_dbg();

	if (vcpu().dispatch_vel2) {
		vcpu().read_vm_registers();

		// Do this after reading the EL1 registers, since they are subsequently
		// overwritten
		vcpu().write_viable_regs();

		vcpu().is_vel2 = true;

		prepare_vel2();

	} else {
		// No need to read viable registers, or write VM registers. The former
		// we read above, the latter are already present in the physical EL1
		// registers.
		vcpu().is_vel2 = false;

		prepare_vm();
	}

	vcpu().restore_program_state();
}

extern "C" void *handle_timer_interrupt()
{
	LOG_INFO("Got a timer int!");

	auto this_core = []() {
		uint64_t v = read_mpidr_el1().v;
		v &= static_cast<uint64_t>(0xff);
		return v;
	}();

	if (network_is_setup()) {
		vmi::receive_and_handle_command();
	}

	return &(&context_stash)[this_core];
}

#ifndef NDEBUG

#define UBSAN_HANDLER(name)                     \
	extern "C" void __ubsan_handle_##name() \
	{                                       \
		die("ubsan " #name);            \
	}

UBSAN_HANDLER(type_mismatch_v1)
UBSAN_HANDLER(pointer_overflow)
UBSAN_HANDLER(out_of_bounds)
UBSAN_HANDLER(load_invalid_value)
UBSAN_HANDLER(divrem_overflow)
UBSAN_HANDLER(shift_out_of_bounds)
UBSAN_HANDLER(add_overflow)
UBSAN_HANDLER(sub_overflow)
UBSAN_HANDLER(builtin_unreachable)

#endif // NDEBUG
