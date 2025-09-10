#include "log.hpp"
#include "registers.hpp"
#include <vcpu_ctx.hpp>

#include <plat/feat.hpp>
#include <sysops.hpp>

#include <dbg/utils.hpp>

/* TODO(multicore) Make thread local */
static struct vcpu_ctx vcpu_ctx[NUM_CORES];

software_thread_id setup_tpidr_el2()
{
	software_thread_id new_tpidr{};
	auto this_core = []() {
		uint64_t v = read_mpidr_el1().v;
		v &= static_cast<uint64_t>(0xff);
		return v;
	}();

	new_tpidr.thread_id =
		reinterpret_cast<uintptr_t>(&(vcpu_ctx[this_core]));

	auto old = read_tpidr_el2();
	write_tpidr_el2(new_tpidr);

	// Prevent subsequent calls to vcpu() to be reordered before the write to
	// TPIDR_EL2
	//
	// Unsure, whether this is necessary, but it does not really hurt either
	COMPILER_BARRIER();

	return old;
}

vncr_el2 translate_vncr_el2(const vncr_el2 in)
{
	auto vncr_ipa = dbg::translate_el1s1(in.v);

    // Cache IPA for later user. TODO: This may be detectable
    vcpu().memory_backend.set_vncr_el2_ipa({vncr_ipa});
	return { vncr_ipa };
}

void vcpu_ctx::read_vm_registers()
{
	// Auxiliary
	memory_backend.set_actlr_el1(read_actlr_el1());
	memory_backend.set_afsr0_el1(read_afsr0_el1());
	memory_backend.set_afsr1_el1(read_afsr1_el1());
	memory_backend.set_amair_el1(read_amair_el1());

	// System Control
	memory_backend.set_sctlr_el1(read_sctlr_el1());
	memory_backend.set_sp_el1(read_sp_el1());
	memory_backend.set_contextidr_el1(read_contextidr_el1());
	memory_backend.set_cpacr_el1(read_cpacr_el1());

	// Generic Timer
	//
	// Do not write or read any CTL/CVAL registers, as timer emulation via the
	// memory backend is broken by the architecture

	memory_backend.set_cntkctl_el1(read_cntkctl_el1());

	// Exceptions
	memory_backend.set_spsr_el1(read_spsr_el1());
	memory_backend.set_esr_el1(read_esr_el1());
	memory_backend.set_far_el1(read_far_el1());
	memory_backend.set_elr_el1(read_elr_el1());
	memory_backend.set_vbar_el1(read_vbar_el1());

	// MMU
	memory_backend.set_tcr_el1(read_tcr_el1());
	memory_backend.set_mair_el1(read_mair_el1());
	memory_backend.set_ttbr0_el1(read_ttbr0_el1());
	memory_backend.set_ttbr1_el1(read_ttbr1_el1());

	// Debug
	memory_backend.set_mdscr_el1(read_mdscr_el1());

	if constexpr (plat::supports(feat::tcr2)) {
		memory_backend.set_tcr2_el1(read_tcr2_el1());
	}

	if constexpr (plat::supports(feat::sctlr2)) {
		memory_backend.set_sctlr2_el1(read_sctlr2_el1());
	}

	if constexpr (plat::supports(feat::spe)) {
		memory_backend.set_pmblimitr_el1(read_pmblimitr_el1());
		memory_backend.set_pmbptr_el1(read_pmbptr_el1());
		memory_backend.set_pmbsr_el1(read_pmbsr_el1());
		memory_backend.set_pmscr_el1(read_pmscr_el1());
		memory_backend.set_pmsevfr_el1(read_pmsevfr_el1());
		memory_backend.set_pmsicr_el1(read_pmsicr_el1());
		memory_backend.set_pmsirr_el1(read_pmsirr_el1());
		memory_backend.set_pmslatfr_el1(read_pmslatfr_el1());
	}

	if constexpr (plat::supports(feat::spe_1p2)) {
		memory_backend.set_pmsnevfr_el1(read_pmsnevfr_el1());
	}

	if constexpr (plat::supports(feat::trf)) {
		memory_backend.set_trfcr_el1(read_trfcr_el1());
	}

	if constexpr (plat::supports(feat::brbe)) {
		memory_backend.set_brbcr_el1(read_brbcr_el1());
	}

	if constexpr (plat::supports(feat::mpam)) {
		memory_backend.set_mpam1_el1(read_mpam1_el1());
	}

	if constexpr (plat::supports(feat::csv2_2)) {
		memory_backend.set_scxtnum_el1(read_scxtnum_el1());
	}

	// TODO
	// Using these fails assembling with
	// `Error: selected processor does not support system register name`
	// so we just don't compile them in for now. We can maybe circumvent this by
	// directly inserting the encoded instructions?
	//
	// See https://stackoverflow.com/a/62267462/3645945

	// if constexpr (plat::supports(feat::mte2)) {
	// 	set_tfsr_el1(read_tfsr_el1());
	// }

	// if constexpr (plat::supports(feat::sve)) {
	// 	set_zcr_el1(read_zcr_el1());
	// }

	// if constexpr (plat::supports(feat::sme)) {
	// 	set_smcr_el1(read_smcr_el1());
	// }

	memory_backend.set_ich_ap0r0_el2(read_ich_ap0r0_el2());
	memory_backend.set_ich_ap1r0_el2(read_ich_ap1r0_el2());

	memory_backend.set_ich_hcr_el2({ read_ich_hcr_el2().v & ~(1UL) });
	memory_backend.set_ich_vmcr_el2(read_ich_vmcr_el2());

	memory_backend.set_ich_lr0_el2(read_ich_lr0_el2());
	memory_backend.set_ich_lr1_el2(read_ich_lr1_el2());
	memory_backend.set_ich_lr2_el2(read_ich_lr2_el2());
	memory_backend.set_ich_lr3_el2(read_ich_lr3_el2());

	memory_backend.set_hcr_el2(memory_backend.hcr_el2()
					   .with_vi(read_hcr_el2().vi())
					   .with_vf(read_hcr_el2().vf()));

	// memory_backend.set_hfgrtr_el2({ 1 << 18 });

	// For NV2
}

void vcpu_ctx::write_vm_registers()
{
    // For NV2 - MUST be done before ttbr writes, as it relies on at instructions
    write_vncr_el2(translate_vncr_el2(memory_backend.vncr_el2()));

	// Auxiliary
	write_actlr_el1(memory_backend.actlr_el1());
	write_afsr0_el1(memory_backend.afsr0_el1());
	write_afsr1_el1(memory_backend.afsr1_el1());
	write_amair_el1(memory_backend.amair_el1());

	// System Control
	write_sctlr_el1(memory_backend.sctlr_el1());
	write_sp_el1(memory_backend.sp_el1());
	write_contextidr_el1(memory_backend.contextidr_el1());
	write_cpacr_el1(memory_backend.cpacr_el1());

	// Generic Timer
	//
	// Do not write or read any CTL/CVAL registers, as timer emulation via the
	// memory backend is broken by the architecture

	write_cntkctl_el1(memory_backend.cntkctl_el1());

	// Exceptions
	write_spsr_el1(memory_backend.spsr_el1());
	write_esr_el1(memory_backend.esr_el1());
	write_far_el1(memory_backend.far_el1());
	write_elr_el1(memory_backend.elr_el1());
	write_vbar_el1(memory_backend.vbar_el1());

	// MMU
	write_tcr_el1(memory_backend.tcr_el1());
	write_mair_el1(memory_backend.mair_el1());
	write_ttbr0_el1(memory_backend.ttbr0_el1());
	write_ttbr1_el1(memory_backend.ttbr1_el1());

	// Debug
	write_mdscr_el1(memory_backend.mdscr_el1());

	if constexpr (plat::supports(feat::tcr2)) {
		write_tcr2_el1(memory_backend.tcr2_el1());
	}

	if constexpr (plat::supports(feat::sctlr2)) {
		write_sctlr2_el1(memory_backend.sctlr2_el1());
	}

	if constexpr (plat::supports(feat::spe)) {
		write_pmblimitr_el1(memory_backend.pmblimitr_el1());
		write_pmbptr_el1(memory_backend.pmbptr_el1());
		write_pmbsr_el1(memory_backend.pmbsr_el1());
		write_pmscr_el1(memory_backend.pmscr_el1());
		write_pmsevfr_el1(memory_backend.pmsevfr_el1());
		write_pmsicr_el1(memory_backend.pmsicr_el1());
		write_pmsirr_el1(memory_backend.pmsirr_el1());
		write_pmslatfr_el1(memory_backend.pmslatfr_el1());
	}

	if constexpr (plat::supports(feat::spe_1p2)) {
		write_pmsnevfr_el1(memory_backend.pmsnevfr_el1());
	}

	if constexpr (plat::supports(feat::trf)) {
		write_trfcr_el1(memory_backend.trfcr_el1());
	}

	if constexpr (plat::supports(feat::brbe)) {
		write_brbcr_el1(memory_backend.brbcr_el1());
	}

	if constexpr (plat::supports(feat::mpam)) {
		write_mpam1_el1(memory_backend.mpam1_el1());
	}

	if constexpr (plat::supports(feat::csv2_2)) {
		write_scxtnum_el1(memory_backend.scxtnum_el1());
	}

	// TODO
	// Using these fails assembling with
	// `Error: selected processor does not support system register name`
	// so we just don't compile them in for now. We can maybe circumvent this by
	// directly inserting the encoded instructions?

	// if constexpr (plat::supports(feat::mte2)) {
	// 	set_tfsr_el1(read_tfsr_el1());
	// }

	// if constexpr (plat::supports(feat::sve)) {
	// 	set_zcr_el1(read_zcr_el1());
	// }

	// if constexpr (plat::supports(feat::sme)) {
	// 	set_smcr_el1(read_smcr_el1());
	// }

	write_ich_hcr_el2({ memory_backend.ich_hcr_el2().v | 1 });
	write_ich_vmcr_el2(memory_backend.ich_vmcr_el2());

	write_ich_ap0r0_el2(memory_backend.ich_ap0r0_el2());
	write_ich_ap1r0_el2(memory_backend.ich_ap1r0_el2());

	write_ich_lr0_el2(memory_backend.ich_lr0_el2());
	write_ich_lr1_el2(memory_backend.ich_lr1_el2());
	write_ich_lr2_el2(memory_backend.ich_lr2_el2());
	write_ich_lr3_el2(memory_backend.ich_lr3_el2());

	// if ((read_ich_lr0_el2().v & (3UL << 62)) && ((read_ich_lr0_el2().v & 0x2005) == 0x2005)) {
	//     printf("going in with the good int!\n");
	// }
	// if ((read_ich_lr1_el2().v & (3UL << 62)) && ((read_ich_lr1_el2().v & 0x2005) == 0x2005)) {
	//     printf("going in with the good int!\n");
	// }
	// if ((read_ich_lr2_el2().v & (3UL << 62)) && ((read_ich_lr2_el2().v & 0x2005) == 0x2005)) {
	//     printf("going in with the good int!\n");
	// }
	// if ((read_ich_lr3_el2().v & (3UL << 62)) && ((read_ich_lr3_el2().v & 0x2005) == 0x2005)) {
	//     printf("going in with the good int!\n");
	// }
}

void vcpu_ctx::write_viable_regs()
{
	// Auxiliary
	write_afsr0_el1(translate_afsr0_el2(memory_backend.afsr0_el2()));
	write_afsr1_el1(translate_afsr1_el2(memory_backend.afsr1_el2()));
	write_amair_el1(translate_amair_el2(memory_backend.amair_el2()));

	// System Control
	write_sctlr_el1(translate_sctlr_el2(memory_backend.sctlr_el2()));
	write_sp_el1(translate_sp_el2(memory_backend.sp_el2()));

	// Hypervisor control
	write_cntkctl_el1(translate_cnthctl_el2(memory_backend.cnthctl_el2()));
	write_cpacr_el1(translate_cptr_el2(memory_backend.cptr_el2()));

	// MMU
	write_mair_el1(translate_mair_el2(memory_backend.mair_el2()));
	write_tcr_el1(translate_tcr_el2(memory_backend.tcr_el2()));
	write_ttbr0_el1(translate_ttbr0_el2(memory_backend.ttbr0_el2()));

	// Exceptions
	write_elr_el1(translate_elr_el2(read_elr_el2()));
	write_spsr_el1(translate_spsr_el2(read_spsr_el2()));
	write_esr_el1(translate_esr_el2(read_esr_el2()));
	write_far_el1(translate_far_el2(read_far_el2()));

	write_vbar_el1(translate_vbar_el2(memory_backend.vbar_el2()));

	isb();
	if (plat::supports(feat::vhe)) {
		write_contextidr_el1(translate_contextidr_el2(
			memory_backend.contextidr_el2()));
		write_ttbr1_el1(
			translate_ttbr1_el2(memory_backend.ttbr1_el2()));
	}
}

void vcpu_ctx::read_viable_regs()
{
	// If we are not E2H, the registers are already present in memory anyways
	if (!hcr_el2().e2h()) {
		return;
	}

	// Auxiliary
	memory_backend.set_afsr0_el2({ read_afsr0_el1().v });
	memory_backend.set_afsr1_el2({ read_afsr1_el1().v });
	memory_backend.set_amair_el2({ read_amair_el1().v });

	// System Control
	memory_backend.set_sctlr_el2({ read_sctlr_el1().v });
	memory_backend.set_sp_el2({ read_sp_el1().v });

	// Hypervisor control
	memory_backend.set_cnthctl_el2({ read_cntkctl_el1().v });
	memory_backend.set_cptr_el2({ read_cpacr_el1().v });

	// MMU
	memory_backend.set_mair_el2(read_mair_el1());
	memory_backend.set_tcr_el2({ read_tcr_el1().v });
	memory_backend.set_ttbr0_el2(read_ttbr0_el1());

	// Exceptions
	memory_backend.set_elr_el2(read_elr_el1());
	memory_backend.set_spsr_el2(read_spsr_el1());
	memory_backend.set_esr_el2(read_esr_el1());
	memory_backend.set_far_el2(read_far_el1());
	memory_backend.set_vbar_el2(read_vbar_el1());

	if (plat::supports(feat::vhe)) {
		memory_backend.set_contextidr_el2(read_contextidr_el1());
		memory_backend.set_ttbr1_el2(read_ttbr1_el1());
	}
}

namespace xlate
{
sctlr translate_sctlr_el2(sctlr input)
{
	// Copy Linux for now
	return sctlr::for_el1()
		.with_mmu(input.mmu())
		.with_alignment_check(input.alignment_check())
		.with_data_cacheability(input.data_cacheability())
		.with_sp_alignment_check(input.sp_alignment_check())
		.with_instruction_cacheability(input.instruction_cacheability())
		.with_iesb(input.iesb())
		.with_write_execute_never(input.write_execute_never())
		.with_big_endian(input.big_endian());
}

xlate_tbl_base translate_ttbr0_el2(xlate_tbl_base ttbr0_el2)
{
	// Copy Linux for now
	return ttbr0_el2.with_asid(0x0);
}

tcr_el1 translate_tcr_el2(tcr_el2 input)
{
	// Copy Linux for now
	return (tcr_el1{})
		.with_epd1(true)
		.with_tbi0(input.tbi())
		.with_ips(input.physical_address_size())
		.with_granule_size(input.granule_size())
		.with_outer_cacheability(input.outer_cacheability())
		.with_inner_cacheability(input.inner_cacheability())
		.with_t0sz(input.t0sz());
}

cntkctl_el1 translate_cnthctl_el2(cnthctl_el2 input)
{
	(void)input;
	todo("nVHE CNTHCTL_EL2");
}

cpacr_el1 translate_cptr_el2(cptr_el2 input)
{
	(void)input;
	todo("nVHE CPTR_EL2");
}
}
