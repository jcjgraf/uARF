#pragma once
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define UARF_IOCTL_RAP _IOWR('m', 1, UarfRapRequest)

typedef struct UarfRapRequest UarfRapRequest;
struct UarfRapRequest {
    union {
        void (*func)(void *);
        void *ptr;
    };
    void *data;
};

static int fd_rap;

static __always_inline void uarf_rap_init(void) {
    fd_rap = open("/dev/rap", O_RDONLY, 0777);
    if (fd_rap == -1) {
        printf("uarf_rap module missing\n");
    }
}

static __always_inline void uarf_rap_deinit(void) {
    close(fd_rap);
}

/**
 * Run `func_p` with `arg` as supervisor
 */
static __always_inline void uarf_rap_call(void *func_p, void *arg) {
    struct UarfRapRequest req = {.ptr = func_p, .data = arg};
    if (ioctl(fd_rap, UARF_IOCTL_RAP, &req) < 0) {
        perror("Failed to rap\n");
    }
}

/**
 * Run `func_p` with `arg` as user
 *
 * Mimics the API of `rap_call`
 */
static __always_inline void uarf_rup_call(void *func_p, void *arg) {
    struct UarfRapRequest req = {.ptr = func_p, .data = arg};
    req.func(req.data);
}
