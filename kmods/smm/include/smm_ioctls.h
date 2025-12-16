#pragma once

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stddef.h>
#include <stdint.h>
#endif

typedef struct ucode ucode;
struct ucode {
    // Pointer to beginning of code region.
    uint64_t ptr;
    // Number of bytes in code region.
    size_t size;
};

#define UARF_SMM_IOCTL_PING     _IOWR('u', 0, uint64_t)
#define UARF_SMM_IOCTL_REGISTER _IOW('u', 1, ucode *)
#define UARF_SMM_IOCTL_RUN      _IO('u', 2)
