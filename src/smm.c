/**
 * Userspace component of SMM kernel module.
 */

#include "smm.h"
#include "lib.h"
#include "smm_ioctls.h"
#include <fcntl.h>
#include <unistd.h>

static int kmod_fd = -1;

int uarf_smm_open(void) {
    if (kmod_fd == -1) {
        kmod_fd = open("/dev/uarf_smm", O_RDWR);
    }
    return kmod_fd < 0 ? kmod_fd : 0;
}

int uarf_smm_close(void) {
    if (kmod_fd <= 0) {
        return 0;
    }

    int ret = close(kmod_fd);
    if (!ret) {
        kmod_fd = 0;
    }
    return ret;
}

uint64_t uarf_smm_ping(uint64_t val) {
    uarf_assert(kmod_fd > 0);

    uint64_t arg = val;

    if (ioctl(kmod_fd, UARF_SMM_IOCTL_PING, &arg)) {
        perror("Failed to ping SMM\n");
    }

    return arg;
}
