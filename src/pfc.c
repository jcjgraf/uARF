#include "pfc.h"
#include "lib.h"
#include "page.h"
#include <err.h>
#include <errno.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

int uarf_pfc_init(UarfPfc *pfc, UarfPfcConfig config) {
    UARF_LOG_TRACE("(%p, 0x%lx)\n", pfc, config.pmu_conf);
    uarf_assert(config.pmu_conf);

    memset(pfc, 0, sizeof(UarfPfc));

    pfc->config = config;

    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    pe.type = config.on_ecore ? 10 : PERF_TYPE_RAW; // Enable E-Core quirks
    pe.size = sizeof(pe);
    pe.config = config.pmu_conf;
    pe.config1 = config.pmu_conf1;
    pe.config2 = config.pmu_conf2;
    // pe.config3 = pfc->config.pmu_conf3; // Not supported on older Linux
    // versions
    pe.sample_type = PERF_SAMPLE_CPU | PERF_SAMPLE_RAW | PERF_SAMPLE_IP;

    pe.disabled = config.start_disabled;

    pe.exclude_user = !!(config.exclude & UARF_PFC_EXCLUDE_USER);
    pe.exclude_kernel = !!(config.exclude & UARF_PFC_EXCLUDE_KERNEL);
    pe.exclude_hv = !!(config.exclude & UARF_PFC_EXCLUDE_HV);
    pe.exclude_idle = !!(config.exclude & UARF_PFC_EXCLUDE_IDLE);
    pe.exclude_host = !!(config.exclude & UARF_PFC_EXCLUDE_HOST);
    pe.exclude_guest = !!(config.exclude & UARF_PFC_EXCLUDE_GUEST);

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
