#pragma once
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

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

static int fd_pi;

static inline void pi_init() {
    fd_pi = open("/dev/pi", O_RDONLY, 0777);
    if (fd_pi == -1) {
        printf("uarf_pi module missing");
    }
}

static inline int pi_deinit() {
    return close(fd_pi);
}

static inline uint64_t pi_rdmsr(uint32_t msr) {
    struct req_msr req = {.msr = msr};

    if (ioctl(fd_pi, IOCTL_RDMSR, &req) < 0) {
        perror("Failed to read MSR\n");
        return -1;
    }

    return req.value;
}

static inline void pi_wrmsr(uint32_t msr, uint64_t value) {
    struct req_msr req = {.msr = msr, .value = value};

    if (ioctl(fd_pi, IOCTL_WRMSR, &req) < 0) {
        perror("Failed to write MSR\n");
    }
}

static inline void pi_invlpg(uint64_t addr) {
    uint64_t a = addr;
    if (ioctl(fd_pi, IOCTL_INVLPG, &a) < 0) {
        perror("Failed to invalidate TLB for addr\n");
    }
}

static inline void pi_flush_tlb(void) {
    if (ioctl(fd_pi, IOCTL_FLUSH_TLB, NULL) < 0) {
        perror("Failed to flush TLB\n");
    }
}

static inline void pi_cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx,
                            uint32_t *edx) {

    struct req_cpuid req = {.eax = leaf, .ebx = 0, .ecx = 0, .edx = 0};

    if (ioctl(fd_pi, IOCTL_CPUID, &req) < 0) {
        perror("Failed to CPUID\n");
    }

    if (eax) {
        *eax = req.eax;
    }
    if (ebx) {
        *ebx = req.ebx;
    }
    if (ecx) {
        *ecx = req.ecx;
    }
    if (edx) {
        *edx = req.edx;
    }
}
