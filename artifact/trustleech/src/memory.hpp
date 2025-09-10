#pragma once

#include <assert.hpp>
#include <types.hpp>
#include <utility.hpp>

namespace memory {
	enum class shareability {
		none = 0b00,
		outer = 0b10,
		inner = 0b11,
	};

	enum class cacheability {
		non_cacheable = 0b00,
		write_back_read_allocate_write_allocate = 0b01,
		write_through_read_allocate_no_write_allocate = 0b10,
		write_back_read_allocate_no_write_allocate = 0b11,
	};

	/** Physical address size for TCR_EL2.PS */
	enum class physical_address_size {
		PAS_48_BITS = 0b101,
	};

	enum class indirect_attribute : uint8_t {
		normal_non_transient_always_write_back_always_allocate = 0xFF,
	};

	enum class stage_2_memory_attributes : uint8_t {
		normal_outer_write_back_inner_write_back = 0b1111,
	};

	assert_size(indirect_attribute, 1);

	constexpr size_t physical_address_space_bytes(const physical_address_size pas) {
		switch (pas) {
			case physical_address_size::PAS_48_BITS:
				return size_t(1) << 48;
		}
	}
}
