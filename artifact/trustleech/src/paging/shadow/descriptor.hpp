#pragma once

#include <assert.hpp>
#include <bitfield.hpp>
#include <paging/types.hpp>
#include <types.hpp>

namespace paging::shadow {
	/** This only works for 4KiB pages */
	union descriptor_shadow {
		BITFIELD_DATA_UINT64(0x0);

		BITFIELD_FIELD_RW(bool, valid, 0, 1);
		BITFIELD_FIELD_RW(enum descriptor_type, descriptor_type, 1, 1);

		/** Bits [47:12] of the descriptor */
		BITFIELD_FIELD_RW(uintptr_t, raw_output_addr, 12, 36);

		constexpr uintptr_t next_level_table_addr(const int level) const {
			(void) level;

			assert(descriptor_type() == descriptor_type::table);
			assert(level >= 0 && level <= 2);

			// The next level table address uses the full 36 bits of the raw
			// output address. We scale it up to the full physical address.
			return raw_output_addr() << raw_output_addr_SHIFT;
		}
      
		constexpr uintptr_t page_addr(const int level) const {
			(void) level;

			assert(descriptor_type() == descriptor_type::page);
			assert(level == 3);

			// The page address uses the full 36 bits of the raw output
			// address. We scale it up to the full physical address.
			return raw_output_addr() << raw_output_addr_SHIFT;
		}

		constexpr uintptr_t block_addr(const int level) const {
			(void) level;

			assert(descriptor_type() == descriptor_type::block);
			assert(level >= 1 && level <= 2);

			const uintptr_t raw_addr = raw_output_addr();

			// The block address uses bits [47:30] for level 1 and [47:21] for
			// level 2. We thus need to filter out the lower bits, since we
			// read [47:12] from the descriptor.
			const uintptr_t deleted_bits = level == 1
				? (1 << 30) - 1
				: (1 << 21) - 1;

			// Scale it up to the full physical address.
			return (raw_addr << raw_output_addr_SHIFT) & ~deleted_bits;
		}
	};

	assert_size(descriptor_shadow, 8);
}
