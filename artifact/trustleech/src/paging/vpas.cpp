#include "plat/qemu/paging.hpp"
#include <paging/vpas.hpp>

#include <exec_vel2.hpp>
#include <plat/paging.hpp>
#include <utility.hpp>

namespace paging::vpas {
	using namespace memory;
	using plat::vpas::address_space;

	namespace {
		static_assert(address_space::LEVEL_1_ENTRIES > 0, "Implementation only supports vPAS paging starting at level 1");

		static_assert(
			plat::VPAS_MMAP.secure_region_size() <= MiB(512),
			"Secure region is larger than 512 MiB (i.e. it can not be covered "
			"by a single 64KiB level 3 table)"
		);
		static_assert(
			level_2_index(plat::VPAS_MMAP.secure_base) == level_2_index(plat::VPAS_MMAP.secure_limit),
			"vPAS Secure region spans multiple level 3 tables"
		);
		static_assert(
			plat::VPAS_MMAP.ram_limit <= TiB(size_t(4)) && level_1_index(plat::VPAS_MMAP.ram_limit) == 0,
			"vPAS limit exceeds 4 TiB (i.e. it can not be covered by a single "
			"64KiB level 2 table"
		);

		using level_1_table = paging::table_64k<1, address_space::LEVEL_1_ENTRIES>;
		using level_2_table = paging::table_64k<2, address_space::LEVEL_2_ENTRIES>;
		using level_3_table = paging::table_64k<3, address_space::LEVEL_3_ENTRIES>;

		level_1_table level_1 {};
		level_2_table level_2 {};
		level_3_table level_3 {};
	}

	void setup() {
		level_1.entries[0] = make_table_descriptor<1>(
			reinterpret_cast<uintptr_t>(&level_2)
		);

		static_assert(
			level_3_index(plat::VPAS_MMAP.secure_base) == 0,
			"Secure region does not start at the beginning of a level 3 table. "
			"This is a simplification for the implementation, making it "
			"unnecessary to handle the case, where there are any non-secure on "
			"the same level 3 table as the secure region."
		);

		/* Use the most permissive attributes so that they are only constrained
		 * by stage 1 mappings. The MMU downgrades the attributes to the less
		 * permissive ones (i.e. if stage 1 designates memory as shareable, but
		 * stage 2 as non-shareable, the memory will be shareable).
		 *
		 * See R_TNHFM, R_GQFSF and R_ZNLRJ
		 */
		const auto make_block_descriptor = [](const uintptr_t addr) {
			return stage_2::make_block_descriptor(
				addr,
				readable::yes,
				writable::yes,
				shareability::none,
				memory::stage_2_memory_attributes::normal_outer_write_back_inner_write_back
			);
		};
		const auto make_page_descriptor = [](const uintptr_t addr) {
			return stage_2::make_page_descriptor(
				addr,
				readable::yes,
				writable::yes,
				shareability::none,
				memory::stage_2_memory_attributes::normal_outer_write_back_inner_write_back
			);
		};
        const auto make_mmio_block_descriptor = [](const uintptr_t addr) {
			return stage_2::make_block_descriptor(
				addr,
				readable::yes,
				writable::yes,
				shareability::none,
                memory::stage_2_memory_attributes{0}
			);
		};
		const auto make_mmio_page_descriptor = [](const uintptr_t addr) {
			return stage_2::make_page_descriptor(
				addr,
				readable::yes,
				writable::yes,
				shareability::none,
                memory::stage_2_memory_attributes{0}
			);
		};


		uintptr_t curr = 0;
		for (; curr < plat::VPAS_MMAP.secure_base; curr += MiB(512)) {
			level_2.entries[level_2_index(curr)] = make_block_descriptor(curr);
		}

		assert(curr == plat::VPAS_MMAP.secure_base);

		level_2.entries[level_2_index(curr)] = make_table_descriptor<2>(
			reinterpret_cast<uintptr_t>(&level_3)
		);

		curr = plat::VPAS_MMAP.secure_limit;
		for (; curr % MiB(512) != 0; curr += paging::PAGE_SIZE_64K) {
			level_3.entries[level_3_index(curr)] = make_page_descriptor(curr);
		}

		for (; curr < plat::VPAS_MMAP.ram_limit; curr += MiB(512)) {
			level_2.entries[level_2_index(curr)] = make_block_descriptor(curr);
		}

        curr = plat::VPAS_MMAP.dma_base;
		for (; curr % MiB(512) != 0; curr += paging::PAGE_SIZE_64K) {
			level_3.entries[level_3_index(curr)] = make_page_descriptor(curr);
		}

		for (; curr < plat::VPAS_MMAP.dma_limit; curr += MiB(512)) {
			level_2.entries[level_2_index(curr)] = make_block_descriptor(curr);
		}
        
        curr = plat::VPAS_MMAP.pci_base;
		for (; curr % MiB(512) != 0; curr += paging::PAGE_SIZE_64K) {
			level_3.entries[level_3_index(curr)] = make_mmio_page_descriptor(curr);
		}

        for (; curr < plat::VPAS_MMAP.pci_base + GiB(1); curr += MiB(512)) {
			level_2.entries[level_2_index(curr)] = make_mmio_block_descriptor(curr);
		}


	}

	stage_2::ctl_regs get_ctl_regs() {
		// In fact, R_SRKBC states that starting at level 1 (when using 64KiB
		// granule) is supported for physical address sizes larger than 44
		// bits, but for simplicity we just use this assertion here.
		static_assert(
			plat::IMPLEMENTED_PHYSICAL_ADDRESS_SIZE == physical_address_size::PAS_48_BITS,
			"According to R_SRKBC, starting at level 1 may not be supported"
		);

		return stage_2::ctl_regs {
			vtcr_el2{}
				.with_t0sz(address_space::SZ)
				.with_sl0(stage_2_starting_level::level_1_64k)
				.with_inner_cacheability(cacheability::write_back_read_allocate_no_write_allocate)
				.with_outer_cacheability(cacheability::write_back_read_allocate_no_write_allocate)
				.with_shareability(shareability::none)
				.with_granule_size(granule_size::G64KiB)
				// TODO(platform) This value is platform dependent on
				// ID_AA64MMFR0_EL1.PARange
				.with_physical_address_size(memory::physical_address_size::PAS_48_BITS),
			vttbr_el2{}
				.with_base_address(reinterpret_cast<uintptr_t>(&level_1))
				.with_vmid(VEL2_VMID)
		};
	}
}
