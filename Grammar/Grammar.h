#pragma once

// This is to get around the fact that bison doesn't make sure this header
// can only be included once.
#ifndef YYTOKENTYPE
#include "Grammar.tab.h"
#endif