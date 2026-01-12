#pragma once

/**
 * List of PM configurations for Intel.
 */

#include "pfc.h"

/**
 * INST_RETIRED.ANY_P
 *
 * This event counts the number of instructions (EOMs) retired. Counting covers
 * macro-fused instructions individually (that is, increments by two).
 */
#define UARF_INTEL_INST_RETIRED_ANY_P UARF_PFC_PMU_CONFIG(.event = 0xC0, .umask = 0x00)

/**
 * INST_RETIRED.PREC_DIST
 *
 * This is a precise version (that is, uses PEBS) of the event that counts instructions
 * retired.
 */
#define UARF_INTEL_INST_RETIRED_PREC_DIST                                                \
    UARF_PFC_PMU_CONFIG(.event = 0xC0, .umask = 0x01)
