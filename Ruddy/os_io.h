#pragma once
 
/* Layer above OS to expose functionality that must be implemented by the operating system */

namespace OS 
{

	extern "C" void _os_printf(wchar_t const* const _Format, ...);

}