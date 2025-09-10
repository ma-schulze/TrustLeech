#include "tail.hpp"

#include <assert.hpp>
#include <context_stash.hpp>
#include <exception/types.hpp>
#include <plat/feat.hpp>
#include <registers.hpp>
#include <sysops.hpp>
#include <vcpu_ctx.hpp>

extern "C" [[noreturn]] void teardown_cpu_asm(const exception_frame*);

namespace {
	void disable_mmu() {
		// Ensure all previous control register writes are completed
		isb();

		write_sctlr_el2(read_sctlr_el2().with_mmu(false));

		isb();
	}
}

[[noreturn]] void teardown_cpu(const exception_frame* frame) {
	assert(frame);

    auto this_core = []() {
      uint64_t v = read_mpidr_el1().v;
      v &= static_cast<uint64_t>(0xff);
      return v;
    }();
	// Transmit values back to the firmware to be restored
	//
	// We can't write SP_EL2 and SCTLR_EL2 directly, as we're currently using
	// our own values
	(&context_stash)[this_core].elr_el3 = vcpu().pc;
	(&context_stash)[this_core].spsr_el3 = vcpu().cpsr.with_el(vcpu().virtual_el()).v;
	(&context_stash)[this_core].sctlr_el2 = vcpu().sctlr_el2().v;
	(&context_stash)[this_core].sp_el2 = vcpu().sp_el2().v;

	// Reverse setup_cpu
	if constexpr (plat::supports(feat::fgt)) {
		write_hdfgwtr_el2(vcpu().hdfgwtr_el2());
		write_hdfgrtr_el2(vcpu().hdfgrtr_el2());
		write_hfgitr_el2(vcpu().hfgitr_el2());
		write_hfgwtr_el2(vcpu().hfgwtr_el2());
		write_hfgrtr_el2(vcpu().hfgrtr_el2());
	}

	if constexpr (plat::supports(feat::hcx)) {
		write_hcrx_el2(vcpu().hcrx_el2());
	}

	if constexpr (plat::supports(feat::vhe)) {
		write_ttbr1_el2(vcpu().ttbr1_el2());
		write_contextidr_el2(vcpu().contextidr_el2());
	}

	write_vncr_el2(vcpu().vncr_el2());

	write_mdcr_el2(vcpu().mdcr_el2());

	write_hacr_el2(vcpu().hacr_el2());
	write_afsr1_el2(vcpu().afsr1_el2());
	write_afsr0_el2(vcpu().afsr0_el2());
	write_actlr_el2(vcpu().actlr_el2());
	// amair_el2 is written later, as we need to turn off the MMU first

	write_cptr_el2(vcpu().cptr_el2());
	write_vttbr_el2(vcpu().vttbr_el2());
	write_vtcr_el2(vcpu().vtcr_el2());
	write_vpidr_el2(vcpu().vpidr_el2());
	write_vmpidr_el2(vcpu().vmpidr_el2());
	write_hstr_el2(vcpu().hstr_el2());
	write_hpfar_el2(vcpu().hpfar_el2());
	write_cnthctl_el2(vcpu().cnthctl_el2());
	write_cntvoff_el2(vcpu().cntvoff_el2());

	// hcr_el2, ttbr0_el2, mair_el2 and tcr_el2 are written later, as we need
	// to turn off the MMU first (due to E2H)

	write_far_el2(vcpu().far_el2());
	write_esr_el2(vcpu().esr_el2());
	write_elr_el2(vcpu().elr_el2());
	write_spsr_el2(vcpu().spsr_el2());
	// We write vbar_el2 late to ensure we have a good exception vector for as
	// long as possible

	// Save these here, as these are viable and overwritten by
	// write_vm_registers
	auto amair_el2 = vcpu().amair_el2();
	auto ttbr0_el2 = vcpu().ttbr0_el2();
	auto mair_el2 = vcpu().mair_el2();
	auto tcr_el2 = vcpu().tcr_el2();

	auto vbar_el2 = vcpu().vbar_el2();

	// If we currently are in vEL2, EL1 registers are only present in the
	// memory page. However, once we return to the unvirtualized system, it
	// expects EL1 registers to be in the physical registers.
	//
	// Do this after writing the EL2 registers above, as some of theme are
	// currently alive in the EL1 registers
	if (vcpu().is_vel2) {
		vcpu().write_vm_registers();
	}

	disable_mmu();

	write_amair_el2(amair_el2);
	write_ttbr0_el2(ttbr0_el2);
	write_mair_el2(mair_el2);
	write_tcr_el2(tcr_el2);

	write_vbar_el2(vbar_el2);

	// TPIDR_EL2 points to the vCPU context, so we need to save any values we
	// need before overwriting
	auto hcr_el2 = vcpu().hcr_el2();
	write_tpidr_el2(vcpu().tpidr_el2());

	tlbi_alle2();

	isb();
	write_hcr_el2(hcr_el2);
	isb();

	teardown_cpu_asm(frame);
}
