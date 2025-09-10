#include "exec_vel2.hpp"

#include <paging/vpas.hpp>
#include <plat/feat.hpp>
#include <registers.hpp>
#include <vcpu_ctx.hpp>

static_assert(
	plat::supports(feat::nv) && plat::supports(feat::nv2),
	"vEL2 requires enhanced nested virtualization"
);

constexpr hcr_el2 VEL2_HCR_EL2_TEMPLATE = (hcr_el2 {})
	// We use the vPAS in vEL2
	.with_vm(true)
	// We only support AArch64
	.with_rw(true)
	.with_apk(plat::supports(feat::pauth))
	.with_api(plat::supports(feat::pauth))
	.with_nv(true)
	.with_nv2(true)
	.with_at(true);

namespace {
	void setup_stage_2_paging() {
		const auto paging_ctl_regs = paging::vpas::get_ctl_regs();

		write_vtcr_el2(paging_ctl_regs.vtcr_el2);
		write_vttbr_el2(paging_ctl_regs.vttbr_el2);
	}

	cnthctl_el2 init_cnthctl_el2() {
		static_assert(
			plat::supports(feat::ecv),
			"We need to be able to trap virtual EL1 timer accesses"
		);

		cnthctl_el2 value {};

		// Unnecessary to trap CNTPCT{,SS}_EL0 accesses - it behaves the same
		// on EL1 as on EL2
		value.ne2h.set_el1pcten(true);
		// However, it is necessary to trap CNTVCT{,SS}_EL0 accesses for E2H
		// guests. They expect the virtual offsets to be 0 on reads, however we
		// unconditionally apply the offset set by vEL2 since it (likely) is
		// required for the timer condition
		value.set_el1tvct(vcpu().hcr_el2().e2h()); // Requires FEAT_ECV

		// Must be trapped for E2H guests in order to forward them to the EL2
		// timers (since thats what the GIC is configured for)
		value.ne2h.set_el1pcen(!vcpu().hcr_el2().e2h());
		value.set_el1tvt(vcpu().hcr_el2().e2h()); // Requires FEAT_ECV

		// Likewise for the EL02 accessors - forward them to the EL1 timers
		value.set_el1nvpct(vcpu().hcr_el2().e2h());
		value.set_el1nvvct(vcpu().hcr_el2().e2h());

		// Unsure about this, since it should only affect operation of the
		// actual EL2, but does not really hurt either - we do not use the
		// event stream for ourselves
		value.set_evnten(vcpu().cnthctl_el2().evnten());
		value.set_evntdir(vcpu().cnthctl_el2().evntdir());
		value.set_evnti(vcpu().cnthctl_el2().evnti());
		value.set_evntis(vcpu().cnthctl_el2().evntis());

		// No need for a physical timer offset
		value.set_ecv(false);

		// Set this to false for my sanity
		value.set_cntvmask(false);
		value.set_cntpmask(false);

		return value;
	}
}

void prepare_vel2() {
	//
	// Hypervisor control base
	//
	write_hcr_el2(
		VEL2_HCR_EL2_TEMPLATE
			.with_nv1(!vcpu().hcr_el2().e2h())
            .with_vf(vcpu().hcr_el2().vf())
            .with_vi(vcpu().hcr_el2().vi())
	);
	write_hstr_el2(hstr_el2 {});

	write_vmpidr_el2(read_mpidr_el1());
	write_vpidr_el2(read_midr_el1());

	setup_stage_2_paging();

	write_cnthctl_el2(init_cnthctl_el2());
	write_cptr_el2(cptr_el2::for_ne2h());

	// write_mdcr_el2(mdcr_el2 {});

	auto vncr = (vncr_el2 {})
		.with_base_address(reinterpret_cast<uintptr_t>(vcpu().memory_backend.get_nv2_redir_area()));
    write_vncr_el2(vncr);


	//
	// FEAT_HCX
	//
	if constexpr (plat::supports(feat::hcx)) {
		write_hcrx_el2({0x800});
	}

	//
	// FEAT_FGT
	//
	// if constexpr (plat::supports(feat::fgt)) {
	// 	write_hfgrtr_el2(hfgrtr_el2 {});
	// 	write_hfgwtr_el2(hfgwtr_el2 {});
	// 	write_hfgitr_el2(hfgitr_el2 {});
	// 	write_hdfgrtr_el2(hdfgrtr_el2 {});
	// 	write_hdfgwtr_el2(hdfgwtr_el2 {});
	// }
}
