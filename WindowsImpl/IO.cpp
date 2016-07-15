#include "stdafx.h"
#include <stdio.h>
#include <fcntl.h>
#include <IO.h>
#include <os_io.h>

namespace OS
{
	extern "C" int _os_printf(wchar_t const* const _Format, ...)
	{
		::_setmode(_fileno(stdout), _O_U16TEXT);
		int _Result;
		va_list _ArgList;
		__crt_va_start(_ArgList, _Format);
		_Result = ::_vfwprintf_s_l(stdout, _Format, NULL, _ArgList);
		__crt_va_end(_ArgList);
		return _Result;
	}
}
