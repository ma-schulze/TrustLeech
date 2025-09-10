#include <exec_vm.hpp>

#include <assert.hpp>
#include <bitfield.hpp>
#include <paging/shadow/shadow.hpp>
#include <paging/vpas.hpp>
#include <plat/feat.hpp>
#include <plat/paging.hpp>
#include <registers.hpp>
#include <sysops.hpp>
#include <vcpu_ctx.hpp>


namespace {
	hcr_el2 translate_hcr_el2(const hcr_el2 in) {
		if (in.tge() && !in.e2h()) {
			die("Setting TGE on returns to EL1 is illegal. Why has this not been caught earlier?");
		}

		if (in.nv() || in.nv2()) {
			// todo("Nested^2 virtualization not yet implemented");
		}

		return in
			// Always either use the vPAS or shadow paging
			.with_vm(true)
			// Data and Instruction Cache disable only affect execution if VM
			// was previously activated. We unconditionally enable VM, so we
			// can not just copy them over
			.with_cd(in.cd() && in.vm())
			.with_id(in.id() && in.vm())
			// While HCR_EL2 is a hypervisor control, E2H actually controls the
			// hypervisor itself so we must be careful
			.with_e2h(false);
	}


	void setup_stage_2_paging() {
		const auto paging_ctl_regs = paging::vpas::should_use()
			? paging::vpas::get_ctl_regs()
			: paging::shadow::get_ctl_regs(vcpu().vttbr_el2(), vcpu().vtcr_el2());

		write_vtcr_el2(paging_ctl_regs.vtcr_el2);
		write_vttbr_el2(paging_ctl_regs.vttbr_el2);
	}

	cnthctl_el2 translate_cnthctl_el2(const cnthctl_el2 in) {
		// We'd have to setup CNTPOFF_EL2 (and maybe other registers)
		assert(!in.ecv());

		if (!vcpu().hcr_el2().e2h()) {
			return in;
		}

		// Copy over bits manually since the format of CNTHCTL_EL2 is wildly
		// different for us (nE2H) vs the guest (E2H)
		auto result = (cnthctl_el2 {})
			// The documentation for this field is:
			// > Enables the generation of an event stream from CNTPCT_EL0 as
			// > seen from EL2.
			// ...so it should only affect operation of EL2. However I am not
			// sure whether this is actually the case, so we copy it over. If,
			// at one point, it can be said for sure this has no effect, it can
			// be removed to minimize guest interference
			.with_evnten(in.evnten())
			.with_evntdir(in.evntdir())
			.with_evnti(in.evnti())
			.with_ecv(in.ecv())
			.with_el1tvt(in.el1tvt())
			.with_el1tvct(in.el1tvct())
			.with_el1nvpct(in.el1nvpct())
			.with_el1nvvct(in.el1nvvct())
			.with_evntis(in.evntis());

		result.ne2h.set_el1pcten(in.e2h.el1pcten());
		result.ne2h.set_el1pcen(in.e2h.el1pten());

		return result;
	}

	constexpr bool should_trap_vm(const fp_trap mode) {
		return mode == fp_trap::trap_el1_el0_1
			|| mode == fp_trap::trap_el1_el0_2;
	}

	cptr_el2 translate_cptr_el2(const cptr_el2 in) {
		if (!vcpu().hcr_el2().e2h()) {
			// This causes some operations (floating point mostly) to be
			// disabled on EL2, i.e. this lets a guest interfere with our
			// operation. However, since we do not use these operations, we can
			// just copy the register over.
			return in;
		}

		// Copy over bits manually since the format of CPTR_EL2 is wildly
		// different for us (nE2H) vs the guest (E2H)
		auto result = cptr_el2::for_ne2h();

		result.ne2h.set_tz(should_trap_vm(in.e2h.zen()));
		result.ne2h.set_tfp(should_trap_vm(in.e2h.fpen()));
		result.ne2h.set_tsm(should_trap_vm(in.e2h.smen()));
		result.ne2h.set_tta(in.e2h.tta());
		result.ne2h.set_tam(in.e2h.tam());
		result.ne2h.set_tcpac(in.e2h.tcpac());

		return result;
	}
}

void prepare_vm() {
	//
	// Hypervisor control base
	//
	write_hcr_el2(translate_hcr_el2(vcpu().hcr_el2()));
	write_hstr_el2(vcpu().hstr_el2());

	write_vmpidr_el2(vcpu().vmpidr_el2());
	write_vpidr_el2(vcpu().vpidr_el2());

	setup_stage_2_paging();

	write_cnthctl_el2(translate_cnthctl_el2(vcpu().cnthctl_el2()));
	write_cptr_el2(translate_cptr_el2(vcpu().cptr_el2()));
    // write_cptr_el2({0x50000000});

	write_mdcr_el2(vcpu().mdcr_el2());

	//
	// FEAT_HCX
	//
	if constexpr (plat::supports(feat::hcx)) {
		write_hcrx_el2({0xc00});
	}
	//
	// FEAT_FGT
	//
	// if constexpr (plat::supports(feat::fgt)) {
	// 	write_hfgrtr_el2(vcpu().hfgrtr_el2());
	// 	write_hfgwtr_el2(vcpu().hfgwtr_el2());
	// 	write_hfgitr_el2(vcpu().hfgitr_el2());
	// 	write_hdfgrtr_el2(vcpu().hdfgrtr_el2());
	// 	write_hdfgwtr_el2(vcpu().hdfgwtr_el2());
	// }


}
