#pragma once

#include <exception/types.hpp>
#include <paging/shadow/descriptor.hpp>
#include <types.hpp>

namespace paging::shadow {
	struct xlate_result {
		bool has_mmu_abort = false;
		mmu_abort_status_code status_code = mmu_abort_status_code::access_flag_l0; // Dummy default value
		shadow::descriptor_shadow output_descriptor { 0 };
		int level = 0;
	};

	xlate_result s2_translate(const uintptr_t ipa);
}
