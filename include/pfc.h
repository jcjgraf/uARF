/**
 * Interact with performance counters
 *
 * Provide functionality and utilities to interact with performance counters in a simple
 * manner.
 */

#pragma once

#include "lib.h"
#include "log.h"
#include <errno.h>
#include <linux/perf_event.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef UARF_LOG_TAG
#undef UARF_LOG_TAG
#define UARF_LOG_TAG UARF_LOG_TAG_PFC
#endif

/**
 * Performance event-select register (PerfEvtSel), used to configure a performance
 * counter. Specifies the event to count, as well as other aspects.
 *
 * From Linux Kernel events/perf_event.h
 */
typedef union UarfPfcEventSel UarfPfcEventSel;
union UarfPfcEventSel {
    struct {
        uint64_t event : 8, umask : 8, usr : 1, os : 1, edge : 1, pc : 1, interrupt : 1,
            __reserved1 : 1, en : 1, inv : 1, cmask : 8, event2 : 4, __reserved2 : 4,
            go : 1, ho : 1;
    } bits;
    uint64_t value;
};

/**
 * Create a PerfEvtSel config.
 *
 * From Linux Kernel events/perf_event.h
 */
#define UARF_PFC_EVENT_SEL(args...) (((UarfPfcEventSel) {.bits = {args}}).value)

/**
 * Configuration for a specific event.
 *
 * Each config is essentially a UarfPfcEventSel.
 *
 * Configurations, c3 is not supported on older kernels
 */
typedef struct UarfPfcEventConfig UarfPfcEventConfig;
struct UarfPfcEventConfig {
    uint64_t config;
    uint64_t config1;
    uint64_t config2;
    // uint64_t config3;
};

/**
 * Create a event config.
 */
#define UARF_PFC_EVENT_CONFIG(args...)                                                   \
    ((UarfPfcEventConfig) {                                                              \
        .config = UARF_PFC_EVENT_SEL(args), .config1 = 0, .config2 = 0})

/**
 * Flags to disable PMU for specific domains.
 */
typedef enum UarfPfcExclude UarfPfcExclude;
enum UarfPfcExclude {
    UARF_PFC_EXCLUDE_USER = BIT(1),
    UARF_PFC_EXCLUDE_KERNEL = BIT(2),
    UARF_PFC_EXCLUDE_HOST = BIT(3),
    UARF_PFC_EXCLUDE_GUEST = BIT(4),
    UARF_PFC_EXCLUDE_HV = BIT(5),
    UARF_PFC_EXCLUDE_IDLE = BIT(6),
};

/**
 * A single performance monitor counter.
 */
typedef struct UarfPfc UarfPfc;
struct UarfPfc {
    // Event selection
    UarfPfcEventConfig event;

    // Comains to exclude
    uint64_t exclude;

    // Have the counter disabled to start.
    // Must not be true when wanting to use rdpmc
    bool start_disabled;

    // Open file descriptor as returned by parf_event_open
    int fd;
    // First page of mmaped ring buffer, containing metadata.
    // The subsequent pages are the actual ringbuffer
    struct perf_event_mmap_page *page;
    // The following fields are relevant when using the rdpmc instruction, as opposed to
    // the read system call, to read the counter. `index` is indicates the desired PMU,
    // while `offset` and `mask` are used to convert the retrieved value to the actual
    // count.
    uint32_t rdpmc_index;
    int64_t rdpmc_offset;
    uint64_t rdpmc_mask;
};

/**
 * Start the PMU described by pfc.
 */
static __always_inline void uarf_pfc_start(UarfPfc *pfc) {
    UARF_LOG_TRACE("(%p)\n", pfc);
    if (ioctl(pfc->fd, PERF_EVENT_IOC_ENABLE, 0) == -1) {
        UARF_LOG_WARNING("Failed to reset: %d\n", errno);
    }
}

/**
 * Stop the PMU described by pfc.
 */
static __always_inline void uarf_pfc_stop(UarfPfc *pfc) {
    // UARF_LOG_TRACE("(%p)\n", pfc); // Having this falsifies the result
    if (ioctl(pfc->fd, PERF_EVENT_IOC_DISABLE, 0) == -1) {
        UARF_LOG_WARNING("Failed to reset: %d\n", errno);
    }
}

/**
 * Reset the PMU described by pfc.
 *
 * Also stops the counter.
 */
static __always_inline void uarf_pfc_reset(UarfPfc *pfc) {
    UARF_LOG_TRACE("(%p)\n", pfc);
    uarf_pfc_stop(pfc);
    if (ioctl(pfc->fd, PERF_EVENT_IOC_RESET, 0) == -1) {
        UARF_LOG_WARNING("Failed to reset: %d\n", errno);
    }
}

/**
 * Get the value of the PMU described by pfc.
 */
static __always_inline uint64_t uarf_pfc_read(UarfPfc *pfc) {
    UARF_LOG_TRACE("(%p)\n", pfc);
    uint64_t count;
    if (read(pfc->fd, &count, sizeof(count)) != sizeof(count)) {
        UARF_LOG_WARNING("Failed to read: %d\n", errno);
    }
    return count;
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

    raw += pfc->rdpmc_offset;
    raw &= pfc->rdpmc_mask;
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

static __always_inline uint64_t uarf_pfc_read_rdpmc(UarfPfc *pfc) {
    // UARF_LOG_TRACE("(%p)\n", pfc); // Adds noise
    uint64_t value = uarf_rdpmc(pfc->rdpmc_index);
    UARF_LOG_DEBUG("raw PFC value: %lu\n", value);
    // value += pfc->offset;
    // value &= pfc->mask;
    return value;
}

/**
 * Helper to open perf.
 */
__always_inline int uarf_perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                                         int group_fd, unsigned long flags) {
    UARF_LOG_TRACE("(%p, %d, %d, %d, %lu)\n", attr, pid, cpu, group_fd, flags);
    return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
}

/*
 * Initialize a PFC
 */
int uarf_pfc_init(UarfPfc *pfc, UarfPfcEventConfig event, uint64_t exclude);

/*
 * De-initialize a PFC
 */
void uarf_pfc_deinit(UarfPfc *pfc);
