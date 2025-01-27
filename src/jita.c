#include "jita.h"
#include "errno.h"
#include "lib.h"

/**
 * Allocate a snip
 */
void _allocate_psnip(jita_ctxt_t *ctxt, stub_t *stub, psnip_t *snip) {
    LOG_TRACE("(%p, %p, %p)\n", ctxt, stub, snip);
    ASSERT(stub);

    ASSERT(snip);
    ASSERT(snip->addr);
    ASSERT(snip->end_addr);

    stub_add(stub, snip->addr, psnip_size(snip));
}

/**
 * Allocate a snip
 */
void _allocate_vsnip(jita_ctxt_t *ctxt, stub_t *stub, vsnip_t *snip) {
    LOG_TRACE("(%p, %p, %p)\n", ctxt, stub, snip);

    ASSERT(stub);
    ASSERT(stub->base_addr);
    ASSERT(stub->addr);
    ASSERT(stub->end_addr);
    ASSERT(stub->base_ptr <= stub->ptr);
    ASSERT(stub->ptr <= stub->end_ptr);

    ASSERT(snip);

    switch (snip->type) {
    case VSNIP_ALIGN: {
        while (vsnip_align_alloc(&snip->vsnip_align, &stub->end_addr,
                                 stub_size_free(stub)) == -ENOSPC) {
            stub_extend(stub);
        }
        break;
    }
    case VSNIP_ASSERT_ALIGN: {
        vsnip_assert_align_alloc(&snip->vsnip_assert_align, stub);
        break;
    }
    case VSNIP_DUMP_STUB: {
        vsnip_dump_stub_alloc(&snip->vsnip_dump_stub, stub);
        break;
    }
    case VSNIP_JMP_NEAR_ABS: {
        while (vsnip_jmp_near_abs_alloc(&snip->vsnip_jmp_near_abs, &stub->end_addr,
                                        stub_size_free(stub)) == -ENOSPC) {
            stub_extend(stub);
        }
        break;
    }
    case VSNIP_JMP_NEAR_REL: {
        while (vsnip_jmp_near_rel_alloc(&snip->vsnip_jmp_near_rel, &stub->end_addr,
                                        stub_size_free(stub)) == -ENOSPC) {
            stub_extend(stub);
        }
        break;
    }
    case VSNIP_FILL: {
        while (vsnip_fill_alloc(&snip->vsnip_fill, &stub->end_addr,
                                stub_size_free(stub)) == -ENOSPC) {
            stub_extend(stub);
        }
        break;
    }
    default: {
        LOG_WARNING("%d is invalid\n", snip->type);
        BUG();
    }
    }
}

void jita_allocate(jita_ctxt_t *ctxt, stub_t *stub, uint64_t addr) {
    LOG_TRACE("(%p, %p, %lx)\n", ctxt, stub, addr);

    ASSERT(ctxt);
    ASSERT(stub);

    ASSERT(addr);

    // Assert stub has not been allocated
    ASSERT(stub->size == 0);

    stub->base_addr = ALIGN_DOWN(addr, PAGE_SIZE);
    stub->size = 0;
    stub->addr = addr;
    stub->end_addr = addr;

    if (ctxt->n_snips == 0) {
        LOG_WARNING("No stubs have been added to the jits!\nIs this really what you want?\n");
    }

    LOG_DEBUG("Create new stub with:\n"
              "\tbase_addr: 0x%lx\n\taddr: 0x%lx\n\tend_addr: 0x%lx\n",
              stub->base_addr, stub->addr, stub->end_addr);

    LOG_DEBUG("There are %lu snippets to allocate\n", ctxt->n_snips);

    for (size_t i = 0; i < ctxt->n_snips; i++) {
        snip_t *snip = &ctxt->snips[i];
        switch (snip->type) {
        case VSNIP:
            _allocate_vsnip(ctxt, stub, &snip->vsnip);
            break;
        case PSNIP:
            _allocate_psnip(ctxt, stub, snip->psnip);
            break;
        default:
            BUG();
        }
    }

    return;
}

void jita_deallocate(jita_ctxt_t *ctxt, stub_t *stub) {
    LOG_TRACE("(%p, %p)\n", ctxt, stub);

    ASSERT(ctxt);
    ASSERT(stub);

    stub_free(stub);
}

void jita_push_vsnip(jita_ctxt_t *ctxt, vsnip_t snip) {
    LOG_TRACE("(%p, snip)\n", ctxt);

    ASSERT(ctxt);
    ASSERT(ctxt->n_snips < JITA_CTXT_MAX_SNIPS - 1);

    ctxt->snips[ctxt->n_snips++] = (snip_t){
        .vsnip = snip,
        .type = VSNIP,
    };

    LOG_DEBUG("There are now %lu/%d snippets\n", ctxt->n_snips, JITA_CTXT_MAX_SNIPS);
}

void jita_push_psnip(jita_ctxt_t *ctxt, psnip_t *snip) {
    LOG_TRACE("(%p, %p)\n", ctxt, snip);
    ASSERT(snip);
    ASSERT(snip->ptr);
    ASSERT(snip->end_ptr);

    ASSERT(ctxt);
    ASSERT(ctxt->n_snips < JITA_CTXT_MAX_SNIPS - 1);

    ctxt->snips[ctxt->n_snips++] = (snip_t){
        .psnip = snip,
        .type = PSNIP,
    };
    LOG_DEBUG("There are now %lu/%d snippets\n", ctxt->n_snips, JITA_CTXT_MAX_SNIPS);
}

void jita_pop(jita_ctxt_t *ctxt, snip_t *snip) {
    LOG_TRACE("(%p, %p)\n", ctxt, snip);
    ASSERT(ctxt);

    // If nothing to pop
    if (!ctxt->n_snips) {
        return;
    }

    // If pop and return
    if (snip) {
        *snip = ctxt->snips[ctxt->n_snips--];
        return;
    }

    // Pop without return
    ctxt->n_snips--;
}

void jita_clone(jita_ctxt_t *from, jita_ctxt_t *to) {
    LOG_TRACE("(%p, %p)\n", from, to);
    *to = *from;
}

void jita_push_vsnip_align(jita_ctxt_t *ctxt, uint32_t align) {
    LOG_TRACE("(%p, %d)\n", ctxt, align);
    ASSERT(ctxt);
    ASSERT(align > 0);

    vsnip_t align_snip = (vsnip_t){
        .type = VSNIP_ALIGN,
        .vsnip_align = (vsnip_align_t){.alignment = align},
    };
    jita_push_vsnip(ctxt, align_snip);
}

void jita_push_vsnip_assert_align(jita_ctxt_t *ctxt, uint32_t alignment) {
    LOG_TRACE("(%p, %u)\n", ctxt, alignment);
    ASSERT(ctxt);

    vsnip_t assert_snip = (vsnip_t){
        .type = VSNIP_ASSERT_ALIGN,
        .vsnip_assert_align = (vsnip_assert_align_t){.alignment = alignment},
    };
    jita_push_vsnip(ctxt, assert_snip);
}

void jita_push_vsnip_dump_stub(jita_ctxt_t *ctxt, stub_t *dump_to) {
    LOG_TRACE("(%p, %p)\n", ctxt, dump_to);
    ASSERT(ctxt);
    ASSERT(dump_to);

    vsnip_t dump_stub_snip = (vsnip_t){
        .type = VSNIP_DUMP_STUB,
        .vsnip_dump_stub = (vsnip_dump_stub_t){.dump_to = dump_to},
    };
    jita_push_vsnip(ctxt, dump_stub_snip);
}

void jita_push_vsnip_jmp_near_abs(jita_ctxt_t *ctxt, uint64_t target_addr) {
    LOG_TRACE("(%p, 0x%lx)\n", ctxt, target_addr);

    ASSERT(ctxt);

    vsnip_t jmp_dir_snip = (vsnip_t){
        .type = VSNIP_JMP_NEAR_ABS,
        .vsnip_jmp_near_abs = (vsnip_jmp_near_abs_t){.target_addr = target_addr},
    };
    jita_push_vsnip(ctxt, jmp_dir_snip);
}

void jita_push_vsnip_jmp_near_rel(jita_ctxt_t *ctxt, uint32_t offset, bool inclusive) {
    LOG_TRACE("(%p, 0x%x, %b)\n", ctxt, offset, inclusive);

    ASSERT(ctxt);

    // Compensate for the jmp instruction size of required
    ASSERT(!inclusive || offset >= 5);
    uint32_t off = inclusive ? offset - 5 : offset;
    LOG_DEBUG("Adjust offset to %u\n", off);

    vsnip_t jmp_dir_snip = (vsnip_t){
        .type = VSNIP_JMP_NEAR_REL,
        .vsnip_jmp_near_rel = (vsnip_jmp_near_rel_t){.offset = off},
    };
    jita_push_vsnip(ctxt, jmp_dir_snip);
}

void jita_push_vsnip_fill_nop(jita_ctxt_t *ctxt, uint32_t num) {
    LOG_TRACE("(%p, %u)\n", ctxt, num);
    ASSERT(ctxt);

    vsnip_t fill_snip = (vsnip_t){
        .type = VSNIP_FILL,
        .vsnip_fill =
            (vsnip_fill_t){
                .bytes = {0x90},
                .size = 1,
                .times = num,
            },
    };
    jita_push_vsnip(ctxt, fill_snip);
}
