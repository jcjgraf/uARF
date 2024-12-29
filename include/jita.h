/*
 * Just-In-Time Assembler allows to dynamically create object code.
 */

#pragma once

#include "log.h"
#include "psnip.h"
#include "stub.h"
#include "vsnip.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG LOG_TAG_JITA
#endif

/**
 * A snippet is either physical or virtual
 */
typedef struct {
    enum snip_t { PSNIP, VSNIP } type;
    union {
        psnip_t *psnip;
        vsnip_t vsnip;
    };
} snip_t;

#define JITA_CTXT_MAX_SNIPS 32
/**
 * Collection of snippets, used to build a stub.
 */
typedef struct {
    snip_t snips[JITA_CTXT_MAX_SNIPS];
    size_t n_snips;
} jita_ctxt_t;

/**
 * Get an initialized jita context
 *
 * Only required in local function, as globals are initialized to zero.
 */
#define jita_get_ctxt()                                                                  \
    (jita_ctxt_t) {                                                                      \
        .n_snips = 0                                                                     \
    }

/**
 * Allocating a context to a stub at a given address.
 *
 * @param ctxt context to allocate
 * @param stub pointer to stub with allocation
 * @param addr address of arbitrary alignment to allocate the context on
 *
 * @NOTE: The stub must not have been alocated previously
 */
void jita_allocate(jita_ctxt_t *ctxt, stub_t *stub, uint64_t addr);

/**
 * Deallocate the stub of a context.
 *
 * @param ctxt not used
 * @param stub stub with allocation
 */
void jita_deallocate(jita_ctxt_t *ctxt, stub_t *stub);

/**
 * Add a virtual snippet to a context.
 */
void jita_push_vsnip(jita_ctxt_t *ctxt, vsnip_t snip);

/**
 * Add a physical snippet to a context.
 */
void jita_push_psnip(jita_ctxt_t *ctxt, psnip_t *snip);

/**
 * Get the last added snip
 */
void jita_pop(jita_ctxt_t *ctxt, snip_t *snip);

/**
 * Clone a jita
 */
void jita_clone(jita_ctxt_t *from, jita_ctxt_t *to);

/**
 * Create a virtual snippet that ensures that the next added snippet is aligned.
 */
void jita_push_vsnip_align(jita_ctxt_t *ctxt, uint32_t align);

/**
 * Create a virtual snippet that asserts that the end of the stub is aligned.
 */
void jita_push_vsnip_assert_align(jita_ctxt_t *ctxt, uint32_t alignment);

/**
 * Create a virtual snippet that dumps the stub at allocation time.
 */
void jita_push_vsnip_dump_stub(jita_ctxt_t *ctxt, stub_t *dump_to);

/**
 * Create a virtual snippet that does a direct jump to `target_addr`.
 */
void jita_push_vsnip_jmp_near_abs(jita_ctxt_t *ctxt, uint64_t target_addr);

/**
 * Create a virtual snippet that does a direct jump with `offset`
 */
void jita_push_vsnip_jmp_near_rel(jita_ctxt_t *ctxt, uint32_t offset);

/**
 * Create a virtual snippet that adds `num` nops
 */
void jita_push_vsnip_fill_nop(jita_ctxt_t *ctxt, uint32_t num);
