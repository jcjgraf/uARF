#pragma once
/**
 * MSRs unique to Intel
 */

// MSR_SMI_COUNT
// 0:31:  SMI Count (R/O), Running count of SMI event since last RESET.
// 32:63: Reserved
#define MSR_SMI_COUNT 0x34
