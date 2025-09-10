#pragma once

#include <types.hpp>

/** Contains the page table used when trustleech itself is running. */
namespace paging::stage_1 {
	void setup();
	uintptr_t get_table();
}
