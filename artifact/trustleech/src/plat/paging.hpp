#pragma once

#include <paging/64k/input_address.hpp>
#include <paging/64k/region.hpp>
#include <types.hpp>

#include <plat/qemu/paging.hpp>

/* Generic part here */

namespace plat::stage_1 {
	using address_space = paging::input_address_info<STAGE_1_ADDRESS_SPACE_SIZE>;
}

namespace plat::vpas {
	using address_space = paging::input_address_info<physical_address_space_bytes(IMPLEMENTED_PHYSICAL_ADDRESS_SIZE)>;
}
