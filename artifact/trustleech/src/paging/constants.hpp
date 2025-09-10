#pragma once

#include <paging/types.hpp>
#include <types.hpp>

namespace paging {
	constexpr size_t PAGE_SIZE_64K = 64 * 1024;

	/** Retrieve the address size, in bits, of a granule
	 *
	 * Modelled after the ARMv8-A ARM DDI 0487K.a
	 * `shared/translation/vmsa/TGxGranuleBits` pseudocode function
	 */
	constexpr uint8_t granule_bits(const granule_size granule) {
		switch (granule) {
			case granule_size::G4KiB:
				return 12;
			case granule_size::G16KiB:
				return 14;
			case granule_size::G64KiB:
				return 16;
		}
	}
}
