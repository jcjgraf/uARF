#pragma once

#include "compiler.h"
#include <stdint.h>
#include <stdlib.h>

#define sfence() asm volatile("sfence" ::: "memory")
#define lfence() asm volatile("lfence" ::: "memory")
#define mfence() asm volatile("mfence" ::: "memory")

#define ASSERT(cond)                                                                     \
    do {                                                                                 \
        if (!(cond)) {                                                                   \
            LOG_WARNING("%s: Assert at %d failed: %s", __func__, __LINE__, STR((cond))); \
            exit(1);                                                                     \
        }                                                                                \
    } while (0)

#define BUG() exit(1)

static inline uint64_t rand47(void) {
    return (_ul(rand()) << 16) ^ rand();
}
