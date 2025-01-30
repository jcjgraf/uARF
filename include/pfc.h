#pragma once

#include "log.h"
#include <linux/perf_event.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG LOG_TAG_PFC
#endif

struct pfc {
    uint64_t config;
    uint64_t config1;
    uint64_t config2;
    uint64_t config3;
    int fd;
    struct perf_event_mmap_page *page;
};

// From Linux Kernel events/perf_event.h
union pmu_config {
    struct {
        uint64_t event : 8, umask : 8, usr : 1, os : 1, edge : 1, pc : 1, interrupt : 1,
            __reserved1 : 1, en : 1, inv : 1, cmask : 8, event2 : 4, __reserved2 : 4,
            go : 1, ho : 1;
    } bits;
    uint64_t value;
};

// From Linux Kernel events/perf_event.h
#define PMU_CONFIG(args...) ((union pmu_config){.bits = {args}}).value

__always_inline int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                                    int group_fd, unsigned long flags) {
    return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
}

/*
 * Initialize a PFC
 */
int pfc_init(struct pfc *pfc);

/*
 * De-initialize a PFC
 */
void pfc_deinit(struct pfc *pfc);

/*
 * Get the value of the PFC
 */
uint64_t pfc_read(struct pfc *pfc);
