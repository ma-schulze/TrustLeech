#pragma once

#include <paging/types.hpp>

namespace paging {
	void* allocate_frame(granule_size size);
}
