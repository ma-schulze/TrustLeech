#pragma once

namespace vmi
{

class GfnAllocator {

public:
    GfnAllocator() = default;
	void *AllocatePage();
	void Free(void *address);
	void *GetNextFreeBlock();
};

extern GfnAllocator allocator;

};
