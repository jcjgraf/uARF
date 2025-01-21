#pragma once

#include <linux/types.h>

#define IOCTL_RDMSR     _IOWR('m', 1, struct req_msr)
#define IOCTL_WRMSR     _IOWR('m', 2, struct req_msr)
#define IOCTL_INVLPG    _IOWR('m', 3, uint64_t)
#define IOCTL_FLUSH_TLB _IOWR('m', 4, uint64_t)
#define IOCTL_CPUID     _IOWR('m', 5, struct req_cpuid)

// For rdmsr/rwmsr
struct req_msr {
    uint32_t msr;
    uint64_t value;
};

struct req_cpuid {
    uint32_t eax; // Leaf
    uint32_t ebx;
    uint32_t ecx; // Sub-leaf
    uint32_t edx;
};
