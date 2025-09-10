#include <paging/stage_1.hpp>

#include <assert.hpp>
#include <paging/types.hpp>
#include <plat/paging.hpp>

namespace paging::stage_1 {
	namespace {
		using plat::stage_1::address_space;

		using level_2_table = paging::table_64k<2, address_space::LEVEL_2_ENTRIES>;
		using level_3_table = paging::table_64k<3, address_space::LEVEL_3_ENTRIES>;

		static_assert(address_space::LEVEL_1_ENTRIES == 0, "Implementation only supports paging starting at level 2");
		static_assert(address_space::LEVEL_2_ENTRIES > 0, "Implementation only supports paging starting at level 2");

		level_2_table level_2 {};

		level_3_table* allocate_level_3_table() {
			// A single, statically allocated table is lent out to the caller.
			// If need be we can use an actual dynamic allocator later.
			static level_3_table table {};
			static bool lent_out = false;

			// This is racey. On the other hand we only call this (and setup)
			// once on startup
			if (!lent_out) {
				lent_out = true;
				return &table;
			}

			die("Out of level 3 tables");
			return nullptr;
		}

		/** This refers to normal, non transient, always write back, always
		 * allocate memory
		 *
		 * TODO Do not use a hardcoded value
		 */
		constexpr uint8_t MAIR_INDEX = 0;

		void map_region(const mmap_region region) {
			/** This is the level 2 block size when using 64KiB pages */
			constexpr size_t L2_BLOCK_SIZE = MiB(512);

			for (size_t curr = region.base; curr < region.limit;) {
				auto& l2_entry = level_2.entries[level_2_index(curr)];

				if (is_aligned(curr, L2_BLOCK_SIZE) && region.encloses(curr + L2_BLOCK_SIZE)) {
					l2_entry = make_block_descriptor(
						curr,
						region.writable,
						region.executable,
						MAIR_INDEX,
						region.shareability
					);

					curr += L2_BLOCK_SIZE;
					continue;
				}

				level_3_table* l3 = nullptr;
				if (is_valid(l2_entry)) {
					l3 = reinterpret_cast<level_3_table*>(next_level_address(l2_entry));
				} else {
					l3 = allocate_level_3_table();
					l2_entry = make_table_descriptor<2>(reinterpret_cast<uintptr_t>(l3));
				}
				assert(l3);

				l3->entries[level_3_index(curr)] = make_page_descriptor(
					curr,
					region.writable,
					region.executable,
					MAIR_INDEX,
					region.shareability
				);

				curr += paging::PAGE_SIZE_64K;
			}
		}
	}

	void setup() {
		for (const mmap_region region : plat::STAGE_1_REGIONS) {
			map_region(region);
		}
	}

	uintptr_t get_table() {
		return reinterpret_cast<uintptr_t>(&level_2);
	}
}
