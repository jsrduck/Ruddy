// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#ifndef NOMINMAX
#define NOMINMAX
#endif
// Has Windows-specific code to load stackmap data section from module
#include <windows.h>
#include <assert.h>
#include <stdlib.h>

// TODO: reference additional headers your program requires here

#define DEBUGPRINT 1