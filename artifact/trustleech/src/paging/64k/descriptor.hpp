#pragma once

#include <memory.hpp>
#include <paging/types.hpp>
#include <types.hpp>
#include <utility.hpp>

namespace paging {
	template<size_t LEVEL>
	struct descriptor_64k {
		static_assert(LEVEL >= 1 && LEVEL <= 3, "Only paging level 1-3 are supported when using 64KiB pages, see R_MLLGN");
		uint64_t v;
	};

#define MAKE_FIELD_FUNCTIONS(macro_name, func_name, param_type) \
	constexpr uint64_t make_ ## func_name (param_type v) { \
		return (static_cast<uint64_t>(v) & (macro_name ## _MASK)) << (macro_name ## _SHIFT); \
	}

	constexpr uint64_t VALID_MASK = 0x1;
	constexpr uint64_t VALID_SHIFT = 0;

	MAKE_FIELD_FUNCTIONS(VALID, valid, const bool);

	constexpr uint64_t DESCRIPTOR_TYPE_MASK = 0x1;
	constexpr uint64_t DESCRIPTOR_TYPE_SHIFT = 1;

	MAKE_FIELD_FUNCTIONS(DESCRIPTOR_TYPE, descriptor_type, const descriptor_type);

	constexpr uint64_t READABLE_MASK = 0x1;
	constexpr uint64_t READABLE_SHIFT = 6;

	MAKE_FIELD_FUNCTIONS(READABLE, readable, const readable);

	constexpr uint64_t WRITABILITY_MASK = 0x1;
	constexpr uint64_t WRITABILITY_SHIFT = 7;

	/** It must be noted, that the meaning of writability is different between
	 * stage 1 and stage 2. In stage 1, `true` means "not writable", while in
	 * stage 2, `true` means "writable".
	 */
	MAKE_FIELD_FUNCTIONS(WRITABILITY, writability, const bool);

	constexpr uint64_t SHAREABILITY_MASK = 0x3;
	constexpr uint64_t SHAREABILITY_SHIFT = 8;

	MAKE_FIELD_FUNCTIONS(SHAREABILITY, shareability, const memory::shareability);

	constexpr uint64_t ACCESS_MASK = 0x1;
	constexpr uint64_t ACCESS_SHIFT = 10;

	MAKE_FIELD_FUNCTIONS(ACCESS, access, const bool);

	constexpr uint64_t DIRTY_BIT_MODIFIER_MASK = 0x1;
	constexpr uint64_t DIRTY_BIT_MODIFIER_SHIFT = 51;

	MAKE_FIELD_FUNCTIONS(DIRTY_BIT_MODIFIER, dirty_bit_modifier, const bool);

	constexpr uint64_t CONTIGUOUS_MASK = 0x1;
	constexpr uint64_t CONTIGUOUS_SHIFT = 52;

	MAKE_FIELD_FUNCTIONS(CONTIGUOUS, contiguous, const bool);

	constexpr uint64_t EXECUTE_NEVER_MASK = 0x1;
	constexpr uint64_t EXECUTE_NEVER_SHIFT = 54;

	MAKE_FIELD_FUNCTIONS(EXECUTE_NEVER, execute_never, const executable);

	constexpr uint64_t TABLE_NEXT_LEVEL_ADDRESS_MASK = 0xFFFFFFFF;
	constexpr uint64_t TABLE_NEXT_LEVEL_ADDRESS_SHIFT = 16;

	MAKE_FIELD_FUNCTIONS(TABLE_NEXT_LEVEL_ADDRESS, next_level_address, const uintptr_t);

	constexpr uint64_t BLOCK_OUTPUT_ADDRESS_MASK = 0x7FFFF;
	constexpr uint64_t BLOCK_OUTPUT_ADDRESS_SHIFT = 29;

	MAKE_FIELD_FUNCTIONS(BLOCK_OUTPUT_ADDRESS, block_output_address, const uintptr_t);

	constexpr uint64_t PAGE_OUTPUT_ADDRESS_MASK = 0xFFFFFFFF;
	constexpr uint64_t PAGE_OUTPUT_ADDRESS_SHIFT = 16;

	MAKE_FIELD_FUNCTIONS(PAGE_OUTPUT_ADDRESS, page_output_address, const uintptr_t);

	template<size_t LEVEL>
	constexpr bool is_valid(const descriptor_64k<LEVEL> d) {
		return bool((d.v >> VALID_SHIFT) & VALID_MASK);
	}

	template<size_t LEVEL>
	constexpr bool is_block_descriptor(const descriptor_64k<LEVEL> d) {
		// Level 3 can only contain page descriptors
		if constexpr (LEVEL == 3) {
			return false;
		}

		const uint64_t value = (d.v >> DESCRIPTOR_TYPE_SHIFT) & DESCRIPTOR_TYPE_MASK;
		return static_cast<descriptor_type>(value) == descriptor_type::block;
	}

	template<size_t LEVEL>
	constexpr uintptr_t next_level_address(const descriptor_64k<LEVEL> d) {
		static_assert(LEVEL != 3, "Table descriptors can not be used on level 3 (see R_MLLGN)");

		if (is_block_descriptor(d)) {
			return 0;
		}

		const uint64_t value = (d.v >> TABLE_NEXT_LEVEL_ADDRESS_SHIFT) & TABLE_NEXT_LEVEL_ADDRESS_MASK;
		return static_cast<uintptr_t>(value << TABLE_NEXT_LEVEL_ADDRESS_SHIFT);
	}

	template<size_t LEVEL>
	constexpr descriptor_64k<LEVEL> make_table_descriptor(const uintptr_t next_level_address) {
		static_assert(LEVEL != 3, "Table Descriptors can not be used on level 3 (see R_MLLGN)");

		return descriptor_64k<LEVEL> {
			  make_valid(true)
			| make_descriptor_type(descriptor_type::table)
			| make_next_level_address(next_level_address >> TABLE_NEXT_LEVEL_ADDRESS_SHIFT)
		};
	}

	namespace stage_1 {
		constexpr uint64_t make_writable(const writable writable) {
			return make_writability(writable == writable::no);
		}

		constexpr uint64_t MAIR_INDEX_MASK = 0x7;
		constexpr uint64_t MAIR_INDEX_SHIFT = 2;

		MAKE_FIELD_FUNCTIONS(MAIR_INDEX, mair_index, const uint8_t);

		constexpr descriptor_64k<2> make_block_descriptor(
			const uintptr_t output_address,
			const writable writable,
			const executable executable,
			const uint8_t mair_index,
			const memory::shareability shareability
		) {
			// Only output block descriptors for level 2, not level 1 (i.e. do
			// not consider FEAT_LPA)
			return descriptor_64k<2> {
				  make_valid(true)
				| make_descriptor_type(descriptor_type::block)
				| make_mair_index(mair_index)
				| make_readable(readable::yes)
				| make_writable(writable)
				| make_shareability(shareability)
				| make_access(true)
				| make_block_output_address(output_address >> BLOCK_OUTPUT_ADDRESS_SHIFT)
				| make_execute_never(executable)
			};
		}

		constexpr descriptor_64k<3> make_page_descriptor(
			const uintptr_t output_address,
			const writable writable,
			const executable executable,
			const uint8_t mair_index,
			const memory::shareability shareability
		) {
			return descriptor_64k<3> {
				  make_valid(true)
				| make_descriptor_type(descriptor_type::page)
				| make_mair_index(mair_index)
				| make_readable(readable::yes)
				| make_writable(writable)
				| make_shareability(shareability)
				| make_access(true)
				| make_page_output_address(output_address >> PAGE_OUTPUT_ADDRESS_SHIFT)
				| make_execute_never(executable)
			};
		}
	}

	namespace stage_2 {
		constexpr uint64_t make_writable(const writable writable) {
			return make_writability(writable == writable::yes);
		}

		constexpr uint64_t MEM_ATTR_MASK = 0x7;
		constexpr uint64_t MEM_ATTR_SHIFT = 2;

		MAKE_FIELD_FUNCTIONS(MEM_ATTR, mem_attr, const memory::stage_2_memory_attributes);

		constexpr descriptor_64k<2> make_block_descriptor(
			const uintptr_t output_address,
			const readable readable,
			const writable writable,
			const memory::shareability shareability,
			const memory::stage_2_memory_attributes mem_attr
		) {
			return descriptor_64k<2> {
				  make_valid(true)
				| make_descriptor_type(descriptor_type::block)
				| make_mem_attr(mem_attr)
				| make_readable(readable)
				| make_writable(writable)
				| make_shareability(shareability)
				| make_access(true)
				| make_block_output_address(output_address >> BLOCK_OUTPUT_ADDRESS_SHIFT)
			};
		}

		constexpr descriptor_64k<3> make_page_descriptor(
			const uintptr_t output_address,
			const readable readable,
			const writable writable,
			const memory::shareability shareability,
			const memory::stage_2_memory_attributes mem_attr
		) {
			return descriptor_64k<3> {
				  make_valid(true)
				| make_descriptor_type(descriptor_type::page)
				| make_mem_attr(mem_attr)
				| make_readable(readable)
				| make_writable(writable)
				| make_shareability(shareability)
				| make_access(true)
				| make_page_output_address(output_address >> PAGE_OUTPUT_ADDRESS_SHIFT)
			};
		}
	}

	template<size_t LEVEL, size_t NUM_ENTRIES>
	struct alignas(sizeof(descriptor_64k<LEVEL>[NUM_ENTRIES])) table_64k {
		static_assert(NUM_ENTRIES <= 8192);
		descriptor_64k<LEVEL> entries[NUM_ENTRIES];
	};
#undef MAKE_FIELD_FUNCTIONS
}
