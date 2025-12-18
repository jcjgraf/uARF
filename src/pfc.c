#include "pfc.h"
#include "lib.h"
#include "page.h"
#include <err.h>
#include <errno.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

int uarf_pfc_init(UarfPfc *pfc, UarfPfcEventConfig event, uint64_t exclude) {
    UARF_LOG_TRACE("(%p, 0x%lx, 0x%lx)\n", pfc, event.config, exclude);
    memset(pfc, 0, sizeof(UarfPfc));
    pfc->event = event;

    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(pe);
    pe.config = pfc->event.config;
    pe.config1 = pfc->event.config1;
    pe.config2 = pfc->event.config2;
    // pe.config3 = pfc->event.config3; // Not supported on older Linux versions
    pe.sample_type = PERF_SAMPLE_CPU | PERF_SAMPLE_RAW | PERF_SAMPLE_IP;

    // Start with the timer disabled. Leads to issues when wanting to use rdpmc,
    // as then the index/offset are not initalised at the time when we extract
    // them.
    // pe.disabled = 1;

    pe.exclude_user = !!(exclude & UARF_PFC_EXCLUDE_USER);
    pe.exclude_kernel = !!(exclude & UARF_PFC_EXCLUDE_KERNEL);
    pe.exclude_hv = !!(exclude & UARF_PFC_EXCLUDE_HV);
    pe.exclude_idle = !!(exclude & UARF_PFC_EXCLUDE_IDLE);
    pe.exclude_host = !!(exclude & UARF_PFC_EXCLUDE_HOST);
    pe.exclude_guest = !!(exclude & UARF_PFC_EXCLUDE_GUEST);

    // pe.precise_ip = 2; // Try to record immediately, but do not enforce
    // pid=0, cpu=-1 => Measure calling process/thread on any CPU

    // TODO: Adjust to allow grouped events
    pfc->fd = uarf_perf_event_open(&pe, 0, -1, -1, PERF_FLAG_FD_NO_GROUP);
    if (pfc->fd == -1) {
        UARF_LOG_ERROR(
            "Error opening PFC 0x%llx: %d (%s)\nDo you run as root?\n",
            pe.config, errno, strerror(errno));
        return -1;
    }

    pfc->page = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, pfc->fd, 0);
    if (pfc->page == MAP_FAILED) {
        UARF_LOG_ERROR("Failed to map PFC page\n");
        return -1;
    }
    if (!pfc->page->cap_user_rdpmc) {
        UARF_LOG_WARNING("rdpmc instruction is not available!\n");
    }

    // Not disabled => index != 0
    uarf_assert(pe.disabled || pfc->page->index != 0);

    pfc->rdpmc_index = pfc->page->index - 1;
    pfc->rdpmc_offset = pfc->page->offset;
    /*
     * If cap_user_rdpmc this field provides the bit-width of the value
     * read using the rdpmc() or equivalent instruction. This can be used
     * to sign extend the result like:
     *
     *   pmc <<= 64 - width;
     *   pmc >>= 64 - width; // signed shift right
     *   count += pmc;
     */
    pfc->rdpmc_mask = (1ul << pfc->page->pmc_width) - 1;

    return 0;
}

void uarf_pfc_deinit(UarfPfc *pfc) {
    UARF_LOG_TRACE("(%p)\n", pfc);
    munmap(pfc->page, PAGE_SIZE);
    close(pfc->fd);
    pfc->page = 0;
    pfc->fd = 0;
}
