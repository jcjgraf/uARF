#pragma once

#include "log.h"
#include <linux/perf_event.h>
#include <stdint.h>
#include <string.h>
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
    uint32_t index;
    int64_t offset;
    uint64_t mask;
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

/*
 * Performance Measurement
 */
struct pm {
    struct pfc pfc;
    // The final count, that gets incremented by pfc_stop
    uint64_t count;
    // Temporary variable used to store that current start value
    uint64_t tmp;
};

/*
 * Performance Measurement Group
 *
 * To track multiple PFCs at the same time
 */
struct pmg {
    // Number PMs that are managed
    size_t num;
    struct pm pms[];
};

// From Linux Kernel events/perf_event.h
#define PMU_CONFIG(args...) ((union pmu_config) {.bits = {args}}).value

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

/*
 * Initialize a measurement for `config`.
 */
static __always_inline int pm_init(struct pm *pm, uint64_t config) {
    LOG_TRACE("(%p, %lu)\n", pm, config);

    memset(pm, 0, sizeof(struct pm));
    pm->pfc.config = config;
    return pfc_init(&pm->pfc);
}

/*
 * De-initialize an initialize measurement
 */
static __always_inline void pm_deinit(struct pm *pm) {
    LOG_TRACE("(%p)\n", pm);
    pfc_deinit(&pm->pfc);
}

/*
 * Start a Measurement
 */
static __always_inline void pm_start(struct pm *pm) {
    LOG_TRACE("(%p)\n", pm);
    pm->tmp = pfc_read(&pm->pfc);
}

/*
 * Stop a measurement
 */
static __always_inline void pm_stop(struct pm *pm) {
    LOG_TRACE("(%p)\n", pm);
    pm->count += pfc_read(&pm->pfc) - pm->tmp;
}

/*
 * Get the value of the measurement
 */
static __always_inline uint64_t pm_get(struct pm *pm) {
    LOG_TRACE("(%p)\n", pm);
    return pm->count;
}

/*
 * Reset the measurement
 */
static __always_inline void pm_reset(struct pm *pm) {
    LOG_TRACE("(%p)\n", pm);
    pm->count = 0;
}

/*
 * Convert the value returned by `rdpmc` to the actual counter value.
 */
static __always_inline uint64_t pm_transform_raw(struct pfc *pfc, uint64_t raw) {
    LOG_TRACE("(%lu)\n", raw);

    // Sign-extend
    // TODO: required?
    // pmc <<= 64 - pfc->page->pmc_width;
    // pmc >>= 64 - pfc->page->pmc_width;

    raw += pfc->offset;
    raw &= pfc->mask;
    return raw;
}
