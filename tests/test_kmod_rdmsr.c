/*
 * Name: test_kmod_rdmsr
 * Desc: Rest the rdmsr kernel module
 */

#include "test.h"
// #include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>

#define DEVICE      "/dev/rdmsr"
#define IOCTL_RDMSR _IOWR('m', 1, struct msr_request)

struct msr_request {
    uint32_t msr;
    uint64_t value;
};

int main(void) {
    int fd;

    struct msr_request req;

    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Module not loaded\n");
        // return -ENODEV;
        return 1;
    }

    req.msr = 0x48;

    if (ioctl(fd, IOCTL_RDMSR, &req) < 0) {
        perror("Failed to read MSR\n");
        // close(fd);
        return 1;
    }

    printf("MSR 0x%x = 0x%lx\n", req.msr, req.value);

    return 0;
}
