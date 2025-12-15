#pragma once

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#include <stddef.h>
#endif

typedef struct ucode ucode;
struct ucode {
    uint64_t ptr;
    size_t size;
};

#define UARF_SMM_IOCTL_PING     _IOWR('u', 0, uint64_t)
#define UARF_SMM_IOCTL_REGISTER _IOW('u', 1, ucode *)
#define UARF_SMM_IOCTL_RUN      _IO('u', 2)
