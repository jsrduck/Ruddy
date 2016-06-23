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
		_Result = ::printf_s(_Format, _ArgList);
		__crt_va_end(_ArgList);
		return _Result;
	}
}
