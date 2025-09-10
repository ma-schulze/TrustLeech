#include "frame_allocator.hpp"

#include <assert.hpp>
#include <bit.hpp>
#include <utility.hpp>

namespace paging {
	namespace {
		constexpr size_t TOTAL_MEMORY = MiB(24);
		constexpr size_t ALLOCATOR_PAGE_SIZE = 4096;
		constexpr size_t TOTAL_PAGES = TOTAL_MEMORY / ALLOCATOR_PAGE_SIZE;
		// Due to the division rounding down, we may not use a few pages at the
		// end. If we would do a ceiling div instead, we would have to be
		// careful, not to use too many pages at the end.
		constexpr size_t TOTAL_USED_LIST_ENTRIES = TOTAL_PAGES / 16;

		using page = uint8_t[ALLOCATOR_PAGE_SIZE];

		alignas(KiB(64)) page memory_stock[TOTAL_PAGES];
		uint16_t used[TOTAL_USED_LIST_ENTRIES];
	}

	void* allocate_frame(granule_size size) {
		switch (size) {
			case granule_size::G16KiB:
				// Not necessary for now. Implementing this requires searching
				// for a 4bit aligned 0000 bitstring in the used array.
				todo("Implement 16KiB page frame allocation");
			case granule_size::G4KiB:
				// Easy to implement by searching for a single free bit
				for (size_t i = 0; i < TOTAL_USED_LIST_ENTRIES; ++i) {
					if (used[i] == 0xFFFF) {
						continue;
					}

					const int first_free = countr_one(used[i]);
					assert(first_free < 16);

					used[i] |= 1 << first_free;
					return &memory_stock[(16 * i) + first_free];
				}
				break;
			case granule_size::G64KiB:
				// Easy to implement by searching for an element of used to be
				// 0. The relevant page will automatically be 64KiB aligned
				for (size_t i = 0; i < TOTAL_USED_LIST_ENTRIES; ++i) {
					if (used[i] == 0) {
						used[i] = 0xFFFF;
						return &memory_stock[16 * i];
					}
				}
				break;
		}

		die("Page frame allocator out of memory");
		return nullptr;
	}
}
