#include "stdafx.h"
#include <IO.h>
#include <cstdio>

namespace OS
{
	extern "C" int _os_printf(char const* const _Format, ...)
	{
		int _Result;
		va_list _ArgList;
		__crt_va_start(_ArgList, _Format);
		_Result = ::_vfprintf_s_l(stdout, _Format, NULL, _ArgList);
		__crt_va_end(_ArgList);
		return _Result;
	}
}
