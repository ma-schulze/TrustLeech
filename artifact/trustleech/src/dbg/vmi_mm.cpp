#include "log.hpp"
#include "vmi.hpp"
#include "vmi_mm.hpp"

#include <assert.hpp>

vmi::GfnAllocator vmi::allocator{};

namespace vmi
{

constexpr int NUM_PAGES = 10;
constexpr int PAGE_SIZE = 4096;
constexpr uint64_t PAGE_SHIFT = 12;

char buffer[NUM_PAGES * PAGE_SIZE] = { 0 }; // 10 pages max
bool pages_used[NUM_PAGES] = { false };

void *GfnAllocator::AllocatePage()
{
	for (int i = 0; i < NUM_PAGES; i++) {
		if (pages_used[i] == false) {
			pages_used[i] = true;
			return (void *)&buffer[i];
		}
	}

	return NULL;
}

void GfnAllocator::Free(void *address)
{
	if (address == NULL) {
		return;
	}
	for (int i = 0; i < NUM_PAGES; i++) {
		if ((uint64_t)&pages_used[i] == (uint64_t)address) {
			pages_used[i] = false;
		}
	}
}

void *GfnAllocator::GetNextFreeBlock()
{
	for (int i = 0; i < NUM_PAGES; i++) {
		if (pages_used[i] == false) {
			return (void *)&buffer[i];
		}
	}

	return NULL;
}

}
