#pragma once

#include <registers.hpp>

namespace paging::stage_2 {
	struct ctl_regs {
		struct vtcr_el2 vtcr_el2;
		struct vttbr_el2 vttbr_el2;

		constexpr bool operator==(const ctl_regs& other) const {
			return vtcr_el2.v == other.vtcr_el2.v
				&& vttbr_el2.v == other.vttbr_el2.v;
		}
	};
}
