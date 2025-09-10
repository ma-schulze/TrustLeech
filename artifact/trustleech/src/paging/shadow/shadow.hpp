#pragma once

#include <exception/mmu.hpp>
#include <paging/stage_2.hpp>
#include <registers.hpp>

namespace paging::shadow {
	/** Setup shadow paging for the given vEL2-produced configuration.
	 *
	 * @return The stage 2 control registers that should be written to the
	 * physical registers for shadow paging.
	 */
	[[nodiscard]] stage_2::ctl_regs get_ctl_regs(vttbr_el2 vel2_vttbr, vtcr_el2 vel2_vtcr);

	void handle_mmu_abort(const exception_syndrome esr, const exception::fault_info& fault_info);

	void clear_cache();
}
