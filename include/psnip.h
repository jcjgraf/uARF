/*
 * Allows the creation of pysical snippets, backed by ASM code, converted to object code
 * at compile time.
 */

#pragma once
#include "page.h"

#ifndef __ASSEMBLY__

#include "compiler.h"
#include <stdint.h>

#define uarf_psnip_start(name) CAT(name, __snip_start)
#define uarf_psnip_end(name)   CAT(name, __snip_end)

#define uarf_psnip_declare(snip_name, local_name)                                        \
    extern char __text uarf_psnip_start(snip_name)[];                                    \
    extern char __text uarf_psnip_end(snip_name)[];                                      \
    UarfPsnip __data local_name = {                                                      \
        .ptr = uarf_psnip_start(snip_name),                                              \
        .end_ptr = uarf_psnip_end(snip_name),                                            \
    }

// clang-format off
#define uarf_psnip_declare_define(name, str)                                                  \
    extern char __text uarf_psnip_start(name)[];                                              \
    extern char __text uarf_psnip_end(name)[];                                                \
    asm(".pushsection .text\n\t"                                                              \
         ".align 0x1000\n\t"                                                                  \
    STR(uarf_psnip_start(name)) ":\n\t"                                                       \
    str                                                                                       \
    STR(uarf_psnip_end(name)) ":\n\t"                                                         \
    "nop\n\t"                                                                                 \
    ".popsection\n\t"                                                                         \
    );                                                                                        \
    UarfPsnip __data name = {                                                                 \
        .ptr = uarf_psnip_start(name),                                                        \
        .end_ptr = uarf_psnip_end(name),                                                      \
    }

#define uarf_psnip_c(name, ret_type, signature, ...)                                            \
    ret_type __section(".text." #name) __used name signature __VA_ARGS__                        \
    void __section(".text." #name) __attribute__((naked)) __used name##__snip_end_func(void) {  \
         asm volatile("nop");                                                                   \
    }                                                                                           \
    asm(".globl " STR(uarf_psnip_start(name)) "\n\t"                                            \
        ".set " STR(uarf_psnip_start(name)) ", " STR(name) "\n\t"                               \
        ".globl " STR(uarf_psnip_end(name)) "\n\t"                                              \
        ".set " STR(uarf_psnip_end(name))   ", " STR(name##__snip_end_func) "\n\t")
// clang-format on

/**
 * Holds pointers to start and end of a snippet
 */
typedef struct UarfPsnip UarfPsnip;
struct UarfPsnip {
    union {
        char *ptr;
        uint64_t addr;
        void (*f)(void);
    };
    union {
        char *end_ptr;
        uint64_t end_addr;
    };
};

/**
 * Get the size of a snippet
 */
static inline uint64_t uarf_psnip_size(UarfPsnip *snip) {
    return snip->end_addr - snip->addr;
}

#else
#include "asm-macros.h"

/* clang-format off */
.macro UARF_SNIP_START name
    SECTION(.text, "", PAGE_SIZE)
    GLOBAL(\name\()__snip_start)
.endm

.macro UARF_SNIP_END name
    .type \name\()__snip_start, STT_FUNC
    SIZE(\name\()__snip_start)
    GLOBAL(\name\()__snip_end)
        nop
.endm
/* clang-format on */
#endif
