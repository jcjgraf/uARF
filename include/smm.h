#pragma once
#include <stddef.h>
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

/**
 * Register new code for the SMM.
 */
int uarf_smm_register(uint64_t ptr, size_t size);

/**
 * Run the registered SMM code.
 */
int uarf_smm_run(void);
