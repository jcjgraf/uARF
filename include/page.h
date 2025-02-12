#pragma once

#include "compiler.h"

#ifndef PAGE_SHIFT
#define PAGE_SHIFT 12
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE (_U64(1) << PAGE_SHIFT)
#endif
