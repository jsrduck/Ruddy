#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <Psapi.h>
#include <intrin.h>  

// Note: At this point, the runtime here is very platform and MSVC specific.
#pragma intrinsic(_ReturnAddress)  

#include "Stackmap.h"
#include "StatepointUtilsApi.h"

void debugprint(wchar_t const* const _Format, ...)
{
#ifdef DEBUGPRINT
	::_setmode(_fileno(stdout), _O_U16TEXT);
	int _Result;
	va_list _ArgList;
	__crt_va_start(_ArgList, _Format);
	_Result = ::_vfwprintf_s_l(stdout, _Format, NULL, _ArgList);
	__crt_va_end(_ArgList);
#endif
}

namespace Runtime {

BYTE* GetStackMapSection(DWORD* sizeOfData)
	{
		debugprint(L"entered MemoryManager\r\n");
		BYTE* dataPtr = nullptr;
		HMODULE hModule = GetModuleHandle(NULL); // module of the exe
		MODULEINFO module_info; 
		memset(&module_info, 0, sizeof(module_info));
		BYTE * module_ptr;
		DWORD module_size;
		if (GetModuleInformation(GetCurrentProcess(), hModule, &module_info, sizeof(module_info)))
		{
			module_size = module_info.SizeOfImage;
			module_ptr = (BYTE*) module_info.lpBaseOfDll;
			debugprint(L"Module:%llx\r\n", module_ptr);
			debugprint(L"ModuleSize:%ul\r\n", module_size);
			debugprint(L"EntryPoint:%llx\r\n", module_info.EntryPoint);
		}
		else
		{
			exit(-99);
		}

		PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER) module_ptr;
		PIMAGE_NT_HEADERS64 ntHeader = (PIMAGE_NT_HEADERS64) ((ULONG_PTR) dosHeader + dosHeader->e_lfanew);
		PIMAGE_FILE_HEADER fileHeader = &ntHeader->FileHeader;
		PIMAGE_OPTIONAL_HEADER64 optionalHeader = &ntHeader->OptionalHeader;

		uintptr_t firstSectionHeader = (uintptr_t) IMAGE_FIRST_SECTION(ntHeader);
		for (int i = 0; i < fileHeader->NumberOfSections; i++)
		{
			PIMAGE_SECTION_HEADER sectionHeader = (PIMAGE_SECTION_HEADER) (firstSectionHeader + i * sizeof(IMAGE_SECTION_HEADER));
#ifdef DEBUGPRINT
			std::string asStr((PCHAR) sectionHeader->Name, IMAGE_SIZEOF_SHORT_NAME);
			std::wstring asWstr(asStr.begin(), asStr.end());
			debugprint(L"\t%s\r\n", asWstr.c_str());
#endif
			if (!strncmp((PCHAR) sectionHeader->Name, ".llvm_st", IMAGE_SIZEOF_SHORT_NAME))
			{
				debugprint(L"\tModule:%llx\r\n", module_ptr);
				debugprint(L"\tModuleSize:%xl\r\n", module_size);
				debugprint(L"\tVirtualAddress:%xl\r\n", sectionHeader->VirtualAddress);
				debugprint(L"\tPhysAddress:%xl\r\n", sectionHeader->Misc.PhysicalAddress);
				debugprint(L"\tRawDataPtr:%xl\r\n", sectionHeader->PointerToRawData);
				dataPtr = module_ptr + sectionHeader->VirtualAddress;
				debugprint(L"\tDataPtr:%xl\r\n", dataPtr);
				*sizeOfData = sectionHeader->SizeOfRawData;
				break;
			}
		}
		return dataPtr;
	}

	class MemoryManager
	{
	public:
		MemoryManager()
		{
			debugprint(L"Entering Memory Manager.\r\n");
			DWORD size;
			BYTE* stackmapSection = GetStackMapSection(&size);
			if (stackmapSection == nullptr)
			{
				debugprint(L"Could not find stackmap, sad...");
				return;
			}
			
			TheStackMapTable = generate_table(stackmapSection);
			debugprint(L"Table generated.\r\n");
			if (TheStackMapTable->empty())
				debugprint(L"Table empty.\r\n");
#ifdef DEBUGPRINT
			for (auto& frame : *(TheStackMapTable))
			{
				DumpFrame(frame.second.get());
			}
#endif
		}

		~MemoryManager()
		{
		}

		StackMapTable TheStackMapTable;
	};

	static MemoryManager s_MemoryManager;

	extern "C" void runtime_safepoint_poll()
	{
		void* stackPtr = _ReturnAddress();
		auto retAddr = (uint64_t) stackPtr;
		debugprint(L"entered safepoint poll\r\n");
		//s_MemoryManager.TheStackMapTable
		if (stackPtr == 0)
		{
			debugprint(L"empty stack maps\r\n");
		}
		if (s_MemoryManager.TheStackMapTable->count(retAddr) == 0)
		{
			debugprint(L"Could not find a frame for this location...\r\n");
			return;
		}

		auto frame = (*s_MemoryManager.TheStackMapTable)[retAddr].get();
		DumpFrame(frame);
	}
}
