#pragma once

#include "log.h"
#include <linux/perf_event.h>
#include <stdint.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef UARF_LOG_TAG
#undef UARF_LOG_TAG
#define UARF_LOG_TAG UARF_LOG_TAG_PFC
#endif

typedef struct UarfPfc UarfPfc;
struct UarfPfc {
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
typedef union UarfPmuConfig UarfPmuConfig;
union UarfPmuConfig {
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
typedef struct UarfPm UarfPm;
struct UarfPm {
    struct UarfPfc pfc;
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
typedef struct UarfPmg UarfPmg;
struct UarfPmg {
    // Number PMs that are managed
    size_t num;
    struct UarfPm pms[];
};

// From Linux Kernel events/perf_event.h
#define UARF_PMU_CONFIG(args...) ((UarfPmuConfig) {.bits = {args}}).value

__always_inline int uarf_perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                                         int group_fd, unsigned long flags) {
    return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
}

/*
 * Initialize a PFC
 */
int uarf_pfc_init(UarfPfc *pfc);

/*
 * De-initialize a PFC
 */
void uarf_pfc_deinit(UarfPfc *pfc);

/*
 * Get the value of the PFC
 */
uint64_t uarf_pfc_read(UarfPfc *pfc);

/*
 * Initialize a measurement for `config`.
 */
static __always_inline int uarf_pm_init(UarfPm *pm, uint64_t config) {
    UARF_LOG_TRACE("(%p, %lu)\n", pm, config);

    memset(pm, 0, sizeof(UarfPm));
    pm->pfc.config = config;
    return uarf_pfc_init(&pm->pfc);
}

/*
 * De-initialize an initialize measurement
 */
static __always_inline void uarf_pm_deinit(UarfPm *pm) {
    UARF_LOG_TRACE("(%p)\n", pm);
    uarf_pfc_deinit(&pm->pfc);
}

/*
 * Start a Measurement
 */
static __always_inline void uarf_pm_start(UarfPm *pm) {
    UARF_LOG_TRACE("(%p)\n", pm);
    pm->tmp = uarf_pfc_read(&pm->pfc);
}

/*
 * Stop a measurement
 */
static __always_inline void uarf_pm_stop(UarfPm *pm) {
    UARF_LOG_TRACE("(%p)\n", pm);
    pm->count += uarf_pfc_read(&pm->pfc) - pm->tmp;
}

/*
 * Get the value of the measurement
 */
static __always_inline uint64_t uarf_pm_get(UarfPm *pm) {
    UARF_LOG_TRACE("(%p)\n", pm);
    return pm->count;
}

/*
 * Reset the measurement
 */
static __always_inline void uarf_pm_reset(UarfPm *pm) {
    UARF_LOG_TRACE("(%p)\n", pm);
    pm->count = 0;
}

/*
 * Convert the combined raw value returned by `rdpmc` to the actual counter value.
 */
static __always_inline uint64_t uarf_pm_transform_raw2(UarfPfc *pfc, uint64_t raw) {
    UARF_LOG_TRACE("(%p, %lu)\n", pfc, raw);

    // Sign-extend
    // TODO: required?
    // pmc <<= 64 - pfc->page->pmc_width;
    // pmc >>= 64 - pfc->page->pmc_width;

    raw += pfc->offset;
    raw &= pfc->mask;
    return raw;
}

/*
 * Convert the separate values returned by `rdpmc` to the actual counter value.
 */
static __always_inline uint64_t uarf_pm_transform_raw1(UarfPfc *pfc, uint32_t lo,
                                                       uint32_t hi) {
    UARF_LOG_TRACE("(%p, %u, %u)\n", pfc, lo, hi);

    return uarf_pm_transform_raw2(pfc, ((uint64_t) hi << 32) | lo);
}
