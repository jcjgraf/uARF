/*
 * Allows the creation of virtual snippets. The object code is generate at runtime.
 */

#pragma once
#include "log.h"
#include "stub.h"
#include <stdint.h>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG LOG_TAG_VSNIP
#endif

#define NOP_BYTE 0x90

/**
 * vsnippet to nop pad to reach an certain alignment
 */
typedef struct {
    uint32_t alignment;
} vsnip_align_t;

/**
 * vsnippet to assert the alignment of the end of the stub
 */
typedef struct {
    uint32_t alignment;
} vsnip_assert_align_t;

/**
 * vsnippet that dumps the stub at the time of allocation
 */
typedef struct {
    stub_t *dump_to;
} vsnip_dump_stub_t;

/**
 * vsnippet that inserts a direct absolute jump to `target_addr`
 */
typedef struct {
    uint64_t target_addr;
} vsnip_jmp_near_abs_t;

/**
 * vsnippet that inserts a direct relative jump by `offset`
 */
typedef struct {
    uint64_t offset;
} vsnip_jmp_near_rel_t;

/**
 * vsnippet that inserts `bytes[:size]` `times`
 */
#define VSNIP_FILL_MAX_SIZE 1
typedef struct {
    uint8_t bytes[VSNIP_FILL_MAX_SIZE];
    uint8_t size;
    uint32_t times;
} vsnip_fill_t;

/**
 * Represents a virtual snippet
 */
typedef struct {
    enum vsnip_t {
        VSNIP_ALIGN,
        VSNIP_ASSERT_ALIGN,
        VSNIP_DUMP_STUB,
        VSNIP_JMP_NEAR_ABS,
        VSNIP_JMP_NEAR_REL,
        VSNIP_FILL,
    } type;
    union {
        vsnip_align_t vsnip_align;
        vsnip_assert_align_t vsnip_assert_align;
        vsnip_dump_stub_t vsnip_dump_stub;
        vsnip_jmp_near_abs_t vsnip_jmp_near_abs;
        vsnip_jmp_near_rel_t vsnip_jmp_near_rel;
        vsnip_fill_t vsnip_fill;
    };
} vsnip_t;

/**
 * Allocation function for vsnip_align_t
 *
 * Insert as many nops to `*base_addr_ptr` to make the updated `*base_addr_ptr` aligned
 * to the requirement of the vsnip_align_t.
 *
 * @param snip pointer to vsnip to allocate
 * @param base_addr_ptr to address where to allocate to. Gets updated to new base
 * address
 * @param rem_size number of bytes that are mapped starting at `*base_addr_ptr`
 *
 * @returns -ENOSPC if `rem_size` is too small to allocate nsnip, else ESUCCESS
 */
int vsnip_align_alloc(vsnip_align_t *snip, uint64_t *base_addr_ptr, uint64_t rem_size);

/**
 * Allocation function for vsnip_assert_align_t
 *
 * Assert that the end of the stub is aligned according to vsnip_assert_align_t
 * Does not actually allocate anything in the stub.
 */
void vsnip_assert_align_alloc(vsnip_assert_align_t *snip, stub_t *stub);

/**
 * Allocation function for vsnip_dump_stub_t
 *
 * Writes the current stub_t to the pointer given in the vsnip_dump_stub_t
 * Does not actually allocate anything in the stub.
 */
void vsnip_dump_stub_alloc(vsnip_dump_stub_t *snip, stub_t *stub);

/**
 * Allocation function for vsnip_jump_near_abs_t
 *
 * @param snip pointer to vsnip to allocate
 * @param base_addr_ptr to address where to allocate to. Gets updated to new base
 * address
 * @param rem_size number of bytes that are mapped starting at `*base_addr_ptr`
 *
 * @returns -ENOSPC if `rem_size` is too small to allocate nsnip, else ESUCCESS
 */
int vsnip_jmp_near_abs_alloc(vsnip_jmp_near_abs_t *snip, uint64_t *base_addr_ptr,
                             uint64_t rem_size);

/**
 * Allocation function for vsnip_jump_near_rel_t
 *
 * @param snip pointer to vsnip to allocate
 * @param base_addr_ptr to address where to allocate to. Gets updated to new base
 * address
 * @param rem_size number of bytes that are mapped starting at `*base_addr_ptr`
 *
 * @returns -ENOSPC if `rem_size` is too small to allocate nsnip, else ESUCCESS
 */
int vsnip_jmp_near_rel_alloc(vsnip_jmp_near_rel_t *snip, uint64_t *base_addr_ptr,
                             uint64_t rem_size);

/**
 * Allocation function for vsnip_fill_t
 *
 * @param snip pointer to vsnip to allocate
 * @param base_addr_ptr to address where to allocate to. Gets updated to new base
 * address
 * @param rem_size number of bytes that are mapped starting at `*base_addr_ptr`
 *
 * @returns -ENOSPC if `rem_size` is too small to allocate nsnip, else ESUCCESS
 */
int vsnip_fill_alloc(vsnip_fill_t *snip, uint64_t *base_addr_ptr, uint64_t rem_size);
