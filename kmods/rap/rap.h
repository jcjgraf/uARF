#pragma once

#include <linux/types.h>

#define UARF_IOCTL_RAP _IOWR('m', 1, UarfRapRequest)

typedef struct UarfRapRequest UarfRapRequest;
struct UarfRapRequest {
    union {
        void (*func)(void *);
        void *ptr;
        uint64_t addr;
    };
    void *data;
};
