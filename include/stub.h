/*
 * Represents executable code. Derived from a jita.
 */

#pragma once
#include <stddef.h>
#include <stdint.h>

/**
 * Executable code, starting at addr
 */
typedef struct stub {
    // Start of allocated memory, aligned to the (initial) size of the allocated memory
    // region
    union {
        char *base_ptr;
        uint64_t base_addr;
    };
    // Size of allocated memory
    size_t size;

    // Start of stub code
    union {
        char *ptr;
        uint64_t addr;
        void (*f)();
    };

    // End of stub, incremented as new snippets are added
    union {
        char *end_ptr;
        uint64_t end_addr;
    };

} stub_t;

/**
 * Add `size` bytes of code at addr `start` to `stub`.
 */
void stub_add(stub_t *stub, uint64_t start, uint64_t size);

/**
 * Map more memory at contiguous addresses for the stub
 */
void stub_extend(stub_t *stub);

/**
 * Frees all memory used by the stub
 */
void stub_free(stub_t *stub);

/**
 * Number of bytes that are free.
 */
uint64_t stub_size_free(stub_t *stub);
