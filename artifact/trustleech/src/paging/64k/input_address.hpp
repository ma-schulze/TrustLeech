#pragma once

#include <bit.hpp>
#include <types.hpp>
#include <utility.hpp>

namespace paging {
	template<size_t INPUT_SPACE_SIZE>
	struct input_address_info {
		/** Number of bits in a virtual address */
		static constexpr uint64_t ADDRESS_BITS = countr_zero(INPUT_SPACE_SIZE);
		/** To be used with the T0SZ/T1SZ fields */
		static constexpr uint64_t SZ = 64 - ADDRESS_BITS;

		static constexpr uint64_t LEVEL_3_ADDRESS_BITS = clamp(ADDRESS_BITS, uint64_t(16), uint64_t(29)) - 16;
		static constexpr uint64_t LEVEL_2_ADDRESS_BITS = clamp(ADDRESS_BITS, uint64_t(29), uint64_t(42)) - 29;
		static constexpr uint64_t LEVEL_1_ADDRESS_BITS = clamp(ADDRESS_BITS, uint64_t(42), uint64_t(48)) - 42;

		static constexpr uint64_t LEVEL_3_ENTRIES = LEVEL_3_ADDRESS_BITS == 0
			? 0 : uint64_t(1) << LEVEL_3_ADDRESS_BITS;
		static constexpr uint64_t LEVEL_2_ENTRIES = LEVEL_2_ADDRESS_BITS == 0
			? 0 : uint64_t(1) << LEVEL_2_ADDRESS_BITS;
		static constexpr uint64_t LEVEL_1_ENTRIES = LEVEL_1_ADDRESS_BITS == 0
			? 0 : uint64_t(1) << LEVEL_1_ADDRESS_BITS;

		static_assert(has_single_bit(INPUT_SPACE_SIZE), "Input address space size is not a power of two");
		static_assert(ADDRESS_BITS >= 25, "Violation of R_PLCGL (TTST is ignored)");
		static_assert(ADDRESS_BITS <= 48, "Violation of R_HYPNC (LVA is ignored)");

		static_assert(LEVEL_3_ENTRIES <= 8192, "Violation of R_MLLGN");
		static_assert(LEVEL_2_ENTRIES <= 8192, "Violation of R_MLLGN");
		static_assert(LEVEL_1_ENTRIES <= 64, "Violation of R_MLLGN");
	};

	constexpr uint16_t level_3_index(uintptr_t addr) {
		constexpr uintptr_t LEVEL_3_MASK = 0x1FFF;
		constexpr uintptr_t LEVEL_3_SHIFT = 16;
		return (addr >> LEVEL_3_SHIFT) & LEVEL_3_MASK;
	}

	constexpr uint16_t level_2_index(uintptr_t addr) {
		constexpr uintptr_t LEVEL_2_MASK = 0x1FFF;
		constexpr uintptr_t LEVEL_2_SHIFT = 29;
		return (addr >> LEVEL_2_SHIFT) & LEVEL_2_MASK;
	}

	constexpr uint16_t level_1_index(uintptr_t addr) {
		constexpr uintptr_t LEVEL_1_MASK = 0x3F;
		constexpr uintptr_t LEVEL_1_SHIFT = 42;
		return (addr >> LEVEL_1_SHIFT) & LEVEL_1_MASK;
	}
}
