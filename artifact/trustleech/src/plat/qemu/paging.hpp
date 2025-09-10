#pragma once

#include <arm-trusted-firmware/plat/qemu/trustleech_exp.h>
#include <memory.hpp>
#include <paging/64k/region.hpp>
#include <types.hpp>

namespace plat {
	/** The size of the address space used for the stage 1 mappings (i.e. those
	 * used when TrustLeech itself is running)
 	 */
	constexpr size_t STAGE_1_ADDRESS_SPACE_SIZE = uint64_t(512) * (1024 * 1024 * 1024);

	/** Regions that should be mapped in the stage 1 mappings (i.e. those used
	 * when TrustLeech itself is running */
	constexpr paging::mmap_region STAGE_1_REGIONS[] = {
        paging::declare_data<0x0, GiB(1)>(),
		paging::declare_read_only<GiB(1), TRUSTLEECH_BASE>(),
		paging::declare_text_region<TRUSTLEECH_TEXT_BASE, TRUSTLEECH_TEXT_LIMIT>(),
		paging::declare_data<TRUSTLEECH_CONTEXT_STASH_BASE, TRUSTLEECH_CONTEXT_STASH_LIMIT>(),
		paging::declare_data<TRUSTLEECH_STACKS_BASE, TRUSTLEECH_STACKS_INT_LIMIT>(),
		paging::declare_read_only<TRUSTLEECH_RODATA_BASE, TRUSTLEECH_RODATA_LIMIT>(),
		paging::declare_data<TRUSTLEECH_RWDATA_BASE, TRUSTLEECH_RWDATA_LIMIT>(),
		paging::declare_data<TRUSTLEECH_LIMIT, TRUSTLEECH_PLAT_RAM_LIMIT>(),
        paging::declare_data<TRUSTLEECH_PCI_BASE, TRUSTLEECH_PCI_LIMIT>(),
		paging::declare_data<TRUSTLEECH_DMA_BASE, TRUSTLEECH_DMA_LIMIT>(),

	};

	/** QEMU actually implements 52-bit physical addresses/FEAT_LPA, but we
	 * only support 48 bits
	 *
	 * According to R_BZHGM, usage of 52-bit physical addresses by a guest
	 * would lead to a stage 2 translation fault
	 */
	constexpr memory::physical_address_size IMPLEMENTED_PHYSICAL_ADDRESS_SIZE = memory::physical_address_size::PAS_48_BITS;

	constexpr paging::vpas_mmap VPAS_MMAP = paging::declare_vpas_mmap<
		TRUSTLEECH_BASE,
		TRUSTLEECH_LIMIT,
		TRUSTLEECH_PLAT_RAM_LIMIT,
        TRUSTLEECH_PCI_BASE, 
        TRUSTLEECH_PCI_LIMIT,
        TRUSTLEECH_DMA_BASE, 
        TRUSTLEECH_DMA_LIMIT
	>();
}
