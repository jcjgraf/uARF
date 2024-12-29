#pragma once
#include "log.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#define MMAP_FLAGS (MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED_NOREPLACE)
#define PROT_RW    (PROT_READ | PROT_WRITE)
#define PROT_RWX   (PROT_RW | PROT_EXEC)

static inline void map_or_die(void *addr_p, size_t size) {
    if (mmap(addr_p, size, PROT_RWX, MMAP_FLAGS, -1, 0) == MAP_FAILED) {
        LOG_ERROR("Failed to map %luB at 0x%lx\n", size, _ul(addr_p));
        exit(1);
    };
    // TODO: use madadise
}

static inline void unmap_or_die(void *addr_p, size_t size) {
    if (munmap(addr_p, size)) {
        LOG_ERROR("Failed to unmap %lu bytees at 0x%lx\n", size, _ul(addr_p));
        exit(1);
    }
}

static inline void *malloc_or_die(size_t size) {
    void *addr = malloc(size);
    if (addr == NULL) {
        LOG_ERROR("Failed to malloc %lu bytes\n", size);
        exit(1);
    }
    return addr;
}

static inline void free_or_die(void *ptr) {
    free(ptr);
}
