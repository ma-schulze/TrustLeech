#pragma once

#include <types.hpp>

namespace paging {
	/** Granule size for TCR_EL2.TG0 */
	enum class granule_size {
		G4KiB = 0b00,
		G64KiB = 0b01,
		G16KiB = 0b10,
	};

	enum class stage_2_starting_level {
		level_3_64k = 0b00,
		level_2_64k = 0b01,
		level_1_64k = 0b10,

		level_2_4k = 0b00,
	};

	/** Specifies what a descriptor points to */
	enum class descriptor_type : uint64_t {
		/** Descriptor points to a block of memory */
		block = 0,
		/** Descriptor points to a lower level table
		 *
		 * To be used in levels higher than 3
		 */
		table = 1,
		/** Descriptor points to a page
		 *
		 * To be used on level 3
		 */
		page = 1,
	};

	enum class readable : uint64_t {
		no = 0,
		yes = 1,
	};

	enum class writable : uint64_t {
		no = 0,
		yes = 1,
	};

	enum class executable : uint64_t {
		no = 1,
		yes = 0,
	};
}
