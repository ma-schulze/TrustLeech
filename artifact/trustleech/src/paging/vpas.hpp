#pragma once

#include <paging/stage_2.hpp>
#include <plat/paging.hpp>
#include <vcpu_ctx.hpp>

/** Types and functions used to implement the virtual physical address space
 * (-> vPAS), which is used to emulate vEL2 and to run EL1 targets where vEL2
 * did not set HCR_EL2.VM.
 */
namespace paging::vpas {
	void setup();
	stage_2::ctl_regs get_ctl_regs();

	/** Whether the vPAS should be active when next dispatching the vCPU. */
	inline bool should_use() {
		return (vcpu().is_vel2 && vcpu().dispatch_vel2)
			|| !vcpu().hcr_el2().vm();
	}

	/** Whether an address is illegal to access in the platform's vPAS. */
	constexpr bool is_illegal(const uintptr_t addr) {
		return plat::VPAS_MMAP.is_illegal(addr);
	}
}
