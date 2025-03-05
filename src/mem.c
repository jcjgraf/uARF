#include "mem.h"
#include <fcntl.h>
#include <unistd.h>

/*
 * Translate `va` of process `pid` to physcial address.
 *
 * If `pid` is 0, use own process.
 */
uint64_t uarf_va_to_pa(uint64_t va, uint64_t pid) {
    UARF_LOG_TRACE("(0x%lx, %lu)\n", va, pid);
    static int fd;
    if (pid) {
        char path[36];
        snprintf(path, sizeof(path), "/proc/%lu/pagemap", pid);
        fd = open(path, O_RDONLY);
    }
    else {
        fd = open("/proc/self/pagemap", O_RDONLY);
    }
    if (!fd) {
        UARF_LOG_ERROR("Failed to convert va to pa. Open pagemap\n");
        exit(1);
    }
    unsigned long pa_with_flags;
    if (pread(fd, &pa_with_flags, 8, (va & ~0xffful) >> 9) < 0) {
        UARF_LOG_ERROR("Failed to convert va to pa. Read pagemap\n");
        exit(1);
    }
    return pa_with_flags << 12 | (va & 0xfff);
}
