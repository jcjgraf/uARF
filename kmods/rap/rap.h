#pragma once

#include <linux/types.h>

#define IOCTL_RAP _IOWR('m', 1, struct rap_request)

struct rap_request {
    union {
        void (*func)(void *);
        void *ptr;
        uint64_t addr;
    };
    void *data;
};
