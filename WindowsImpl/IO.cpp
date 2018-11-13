#include "stdafx.h"
#include <stdio.h>
#include <fcntl.h>
#include <IO.h>
#include <os_io.h>

namespace OS
{
	extern "C" void _os_printf(wchar_t const* const _Format, ...)
	{
		::_setmode(_fileno(stdout), _O_U16TEXT);
		int _Result;
		va_list _ArgList;
		__crt_va_start(_ArgList, _Format);
		_Result = ::_vfwprintf_s_l(stdout, _Format, NULL, _ArgList);
		__crt_va_end(_ArgList);
		// returning void - a current limitation of the gc-statepoint pass we're using
		// is that it doesn't support non-void vararg functions. Since nobody usually cares
		// about the return result for printf, we're going to swallow the result for now.
		//return _Result;
	}
}
