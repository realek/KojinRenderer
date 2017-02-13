#pragma once
#include <stdlib.h>

namespace MemoryAllocation
{
	void* AllocateAligned(size_t size, size_t alignment)
	{
		void* data = nullptr;
#if defined(_WIN32)
		data = _aligned_malloc(size, alignment);
#else 
		//make linux code here
#endif
		return data;
	}

	void FreeAligned(void* data)
	{
#if	defined(_WIN32)
		_aligned_free(data);
#else 
		free(data);
#endif
	}
}

