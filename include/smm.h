#pragma once
#include <stdint.h>

/**
 * Open the kernel module.
 *
 * @returns Whether the kernel module was opened successfully.
 */
int uarf_smm_open(void);

/**
 * Close the kernel module.
 *
 * @returns Whether the kernel module was closed successfully.
 */
int uarf_smm_close(void);

/**
 * Ping the SMM by sending a value that gets modified in the SMM.
 */
uint64_t uarf_smm_ping(uint64_t val);
