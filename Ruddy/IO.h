#pragma once
 
/* Layer above OS to expose functionality that must be implemented by the operating system */

namespace OS 
{

	extern "C" int _os_printf(char const* const _Format, ...);

}