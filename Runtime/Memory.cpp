#include "stdafx.h"
#include "Runtime.h"

namespace Runtime {

extern "C" void* _heap_alloc(size_t size)
{
	debugprint(L"allocating %u bytes on heap\r\n", size);
	// For now, just malloc it
	return ::malloc(size);
}

}