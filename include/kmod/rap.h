#pragma once
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define IOCTL_RAP _IOWR('m', 1, struct rap_request)

struct rap_request {
    union {
        void (*func)(void *);
        void *ptr;
    };
    void *data;
};

static int fd_rap;

static inline void rap_init(void) {
    fd_rap = open("/dev/rap", O_RDONLY, 0777);
    if (fd_rap == -1) {
        printf("uarf_rap module missing\n");
    }
}

static inline void rap_deinit(void) {
    close(fd_rap);
}

/**
 * Run `func_p` with `arg` as supervisor
 */
static inline void rap_call(void *func_p, void *arg) {
    struct rap_request req = {.ptr = func_p, .data = arg};
    if (ioctl(fd_rap, IOCTL_RAP, &req) < 0) {
        perror("Failed to rap\n");
    }
}

/**
 * Run `func_p` with `arg` as user
 *
 * Mimics the API of `rap_call`
 */
static inline void rup_call(void *func_p, void *arg) {
    struct rap_request req = {.ptr = func_p, .data = arg};
    req.func(req.data);
}
