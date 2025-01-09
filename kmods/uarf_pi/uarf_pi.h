#pragma once

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#define DEVICE_NAME     "uarf_pi"
#define IOCTL_RDMSR     _IOWR('m', 1, struct msr_request)
#define IOCTL_WRMSR     _IOWR('m', 2, struct msr_request)
#define IOCTL_INVLPG    _IOWR('m', 3, uint64_t)
#define IOCTL_FLUSH_TLB _IOWR('m', 4, uint64_t)

// For rdmsr/rwmsr
struct msr_request {
    uint32_t msr;
    uint64_t value;
};

#ifndef __KERNEL__
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MSR_PRET_CMD 0x49

#define IBPB() pi_wrmsr(MSR_PRET_CMD, 1);

static int fd_pi;

static inline void pi_init() {
    fd_pi = open("/dev/" DEVICE_NAME, O_RDONLY, 0777);
    if (fd_pi == -1) {
        printf("uarf_pi module missing");
    }
}

static inline int pi_deinit() {
    return close(fd_pi);
}

static inline uint64_t pi_rdmsr(uint32_t msr) {
    struct msr_request req = {.msr = msr};

    if (ioctl(fd_pi, IOCTL_RDMSR, &req) < 0) {
        perror("Failed to read MSR\n");
        return -1;
    }

    return req.value;
}

static inline void pi_wrmsr(uint32_t msr, uint64_t value) {
    struct msr_request req = {.msr = msr, .value = value};

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
#endif
