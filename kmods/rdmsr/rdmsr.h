#pragma once

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#define DEVICE_NAME "rdmsr"
#define IOCTL_RDMSR _IOWR('m', 1, struct msr_request)
#define IOCTL_WRMSR _IOWR('m', 2, struct msr_request)

struct msr_request {
    uint32_t msr;
    uint64_t value;
};

#ifndef __KERNEL__
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int fd_rdmsr;

static inline void msr_init() {
    fd_rdmsr = open("/dev/" DEVICE_NAME, O_RDONLY, 0777);
    if (fd_rdmsr == -1) {
        printf("rdmsr module missing");
    }
}

static inline uint64_t msr_rdmsr(uint32_t msr) {
    struct msr_request req = {.msr = msr};

    if (ioctl(fd_rdmsr, IOCTL_RDMSR, &req) < 0) {
        perror("Failed to read MSR\n");
        close(fd_rdmsr);
        // TODO: Handle error
        return -1;
    }

    return req.value;
}

static inline void msr_wrmsr(uint32_t msr, uint64_t value) {
    struct msr_request req = {.msr = msr, .value = value};

    if (ioctl(fd_rdmsr, IOCTL_WRMSR, &req) < 0) {
        perror("Failed to write MSR\n");
        close(fd_rdmsr);
        // TODO: Handle error
        // return -1;
    }
}
#endif
