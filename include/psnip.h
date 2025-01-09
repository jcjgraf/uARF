/*
 * Allows the creation of pysical snippets, backed by ASM code, converted to object code
 * at compile time.
 */

#pragma once
#include "page.h"

#ifndef __ASSEMBLY__

#include "compiler.h"
#include <stdint.h>

#define psnip_start(name) CAT(name, __snip_start)
#define psnip_end(name)   CAT(name, __snip_end)

#define psnip_declare(snip_name, local_name)                                             \
    extern char __text psnip_start(snip_name)[];                                         \
    extern char __text psnip_end(snip_name)[];                                           \
    psnip_t __data local_name = {                                                        \
        .ptr = psnip_start(snip_name),                                                   \
        .end_ptr = psnip_end(snip_name),                                                 \
    }

/**
 * Holds pointers to start and end of a snippet
 */
typedef struct {
    union {
        char *ptr;
        uint64_t addr;
        void (*f)();
    };
    union {
        char *end_ptr;
        uint64_t end_addr;
    };
} psnip_t;

/**
 * Get the size of a snippet
 */
static inline uint64_t psnip_size(psnip_t *snip) {
    return snip->end_addr - snip->addr;
}

#else
#include "asm-macros.h"

/* clang-format off */
.macro SNIP_START name
    SECTION(.text, "", PAGE_SIZE)
    GLOBAL(\name\()__snip_start)
.endm

.macro SNIP_END name
    .type \name\()__snip_start, STT_FUNC
    SIZE(\name\()__snip_start)
    GLOBAL(\name\()__snip_end)
        nop
.endm
/* clang-format on */
#endif
