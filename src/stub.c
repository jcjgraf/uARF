#include "stub.h"
#include "lib.h"
#include "log.h"
#include "mem.h"

#include "page.h"
#include <string.h>

void stub_add(stub_t *stub, uint64_t start, uint64_t size) {
    LOG_TRACE("(%p, 0x%lx, %lu)\n", stub, start, size);

    ASSERT(stub);
    ASSERT(stub->base_addr);
    ASSERT(stub->addr);
    ASSERT(stub->end_addr);
    ASSERT(stub->base_ptr <= stub->ptr);
    ASSERT(stub->ptr <= stub->end_ptr);

    if (size == 0) {
        return;
    }

    // Check if enough space left
    while (stub_size_free(stub) < size) {
        stub_extend(stub);
    }

    LOG_DEBUG("Copy code of size %lu to 0x%lx\n", size, stub->end_addr);

    memcpy(stub->end_ptr, _ptr(start), size);
    stub->end_ptr += size;
    ASSERT(stub->end_addr <= stub->base_addr + stub->size);
}

void stub_extend(stub_t *stub) {
    LOG_TRACE("(%p)\n", stub);
    ASSERT(stub);
    ASSERT(stub->base_addr);
    ASSERT(stub->addr);
    ASSERT(stub->end_addr);
    ASSERT(stub->base_ptr <= stub->ptr);
    ASSERT(stub->ptr <= stub->end_ptr);

    uint64_t next_page = stub->base_addr + stub->size;
    LOG_DEBUG("Map one more page starting at 0x%lx\n", next_page);
    // vmap_kern_4k_or_die(_ptr(next_page), get_free_frame()->mfn, L1_PROT);
    map_or_die(_ptr(next_page), PAGE_SIZE);
    stub->size += PAGE_SIZE;
}

void stub_free(stub_t *stub) {
    LOG_TRACE("(%p)\n", stub);
    ASSERT(stub);
    ASSERT(stub->base_addr);
    ASSERT(stub->addr);
    ASSERT(stub->end_addr);
    ASSERT(stub->base_ptr <= stub->ptr);
    ASSERT(stub->ptr <= stub->end_ptr);

    uint64_t current_page = stub->base_addr;
    size_t remaining_size = stub->size;

    LOG_DEBUG("There are %lu B to unmap\n", remaining_size);

    while (remaining_size) {
        LOG_DEBUG("Unmap page at 0x%lx\n", current_page);
        unmap_or_die(_ptr(current_page), PAGE_SIZE);
        remaining_size -= PAGE_SIZE;
        current_page += PAGE_SIZE;
    }

    stub->size = 0;
    stub->base_ptr = 0;
    stub->addr = 0;
    stub->end_addr = 0;
}

uint64_t stub_size_free(stub_t *stub) {
    LOG_TRACE("(%p)\n", stub);

    ASSERT(stub);
    ASSERT(stub->base_addr);
    ASSERT(stub->addr);
    ASSERT(stub->end_addr);
    ASSERT(stub->base_ptr <= stub->ptr);
    ASSERT(stub->ptr <= stub->end_ptr);

    if (stub->size == 0) {
        return 0;
    }

    uint64_t stub_end = stub->base_addr + stub->size;

    ASSERT(stub_end >= stub->end_addr);

    return stub->base_addr + stub->size - stub->end_addr;
}
