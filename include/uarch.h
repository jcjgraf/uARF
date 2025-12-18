#pragma once
/**
 * Define macros that depend on the microarchitecture.
 */

#include "compiler.h"

#define UARF_INTEL 0x100
#define ALDER_LAKE (UARF_INTEL + 0x12)

#define UARF_AMD 0x200
#define ZEN4     (UARF_AMD + 0x4)

// If the microarchitecture of the current platform is equal to uarch.
#define UARF_IS_UARCH(uarch) (uarch == UARCH)

#define UARF_IS_INTEL() (UARCH & UARF_INTEL)
#define UARF_IS_AMD()   (UARCH & UARF_AMD)

#pragma message "arch: " STR(UARCH)
#if UARF_IS_INTEL()
#pragma message "Compile for Intel"
#elif UARF_IS_AMD()
#pragma message "Compile for AMD"
#else
#error "Unknown vendor: " STR(UARCH)
#endif

#if !UARF_IS_UARCH(ALDER_LAKE) && !UARF_IS_UARCH(ZEN4)
#error "Unknown system"
#endif
