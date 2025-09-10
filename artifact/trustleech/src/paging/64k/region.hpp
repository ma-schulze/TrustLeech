#pragma once

#include <paging/64k/descriptor.hpp>
#include <paging/constants.hpp>
#include <types.hpp>

namespace paging {
	struct mmap_region {
		uintptr_t base;
		uintptr_t limit;
		paging::writable writable;
		paging::executable executable;
		memory::shareability shareability;

		constexpr bool encloses(uintptr_t addr) const {
			return base <= addr && addr <= limit;
		}
	};

	template<
		uintptr_t base,
		uintptr_t limit,
		paging::writable writable,
		paging::executable executable,
		memory::shareability shareability
	>
	constexpr mmap_region declare_region() {
		static_assert(is_aligned(base, paging::PAGE_SIZE_64K), "mmap_region base is not aligned to page border");
		static_assert(is_aligned(limit, paging::PAGE_SIZE_64K), "mmap_region limit is not aligned to page border");
		static_assert(base < limit, "Region limit is <= base");

		return mmap_region { base, limit, writable, executable, shareability };
	}

	template<uintptr_t base, uintptr_t limit>
	constexpr mmap_region declare_text_region() {
		return declare_region<base, limit, paging::writable::no, paging::executable::yes, memory::shareability::none>();
	}

	template<uintptr_t base, uintptr_t limit>
	constexpr mmap_region declare_read_only() {
		return declare_region<base, limit, paging::writable::no, paging::executable::no, memory::shareability::none>();
	}

	template<uintptr_t base, uintptr_t limit>
	constexpr mmap_region declare_data() {
		return declare_region<base, limit, paging::writable::yes, paging::executable::no, memory::shareability::outer>();
	}

	/** Defines the memory map of the vPAS
	 *
	 * [secure_base, secure_limit) is the memory region that is marked
	 * virtually secure. During initialization, the memory area TrustLeech is
	 * loaded to is remapped from secure to non-secure, since TrustLeech
	 * operates at N-EL2 once active. To prevent vEL2, or EL1, from
	 * subsequently accessing the memory, the memory can be marked as virtually
	 * secure in the stage 2 translation.
	 *
	 * The areas [0, secure_base) and [secure_limit, ram_limit) are simply
	 * identity mapped.
	 */
	struct vpas_mmap {
		uintptr_t secure_base;
		uintptr_t secure_limit;
		uintptr_t ram_limit;
        uintptr_t pci_base;
        uintptr_t pci_limit;
        uintptr_t dma_base;
        uintptr_t dma_limit;

		constexpr size_t secure_region_size() const {
			return secure_limit - secure_base;
		}

        constexpr size_t dma_region_size() const {
			return dma_limit - dma_base;
		}

        constexpr size_t pci_region_size() const {
			return pci_limit - pci_base;
		}


		constexpr bool is_illegal(uintptr_t addr) const {
			return (secure_base <= addr && addr < secure_limit)
				|| (addr >= ram_limit);
		}
	};

	template<
		uintptr_t secure_base,
		uintptr_t secure_limit,
		uintptr_t ram_limit,
        uintptr_t pci_base,
        uintptr_t pci_limit,
        uintptr_t dma_base,
        uintptr_t dma_limit
	>
	constexpr vpas_mmap declare_vpas_mmap() {
		static_assert(is_aligned(secure_base, paging::PAGE_SIZE_64K), "vpas_mmap secure_base is not aligned to page border");
		static_assert(is_aligned(secure_limit, paging::PAGE_SIZE_64K), "vpas_mmap secure_limit is not aligned to page border");
		static_assert(is_aligned(ram_limit, paging::PAGE_SIZE_64K), "vpas_mmap ram_limit is not aligned to page border");
		static_assert(secure_base < secure_limit, "Secure region limit is <= base");
		static_assert(secure_limit < ram_limit, "Secure region limit is >= ram_limit");

		return vpas_mmap { secure_base, secure_limit, ram_limit, pci_base, pci_limit, dma_base, dma_limit };
	}
}
