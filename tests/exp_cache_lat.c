/**
 * Measure Cache Latency
 *
 * Determine the times required to access different cache levels.
 * This measurement is dependent on the measurement method.
 *
 * Zen4 (incl. Overhead):
 * - L1: 49
 * - L2: 56
 * - L3: 115
 * - Mem: 468
 *
 * The measurement overhead is 48.

 * Zen4 (minus overhead):
 * - L1: 1
 * - L2: 8
 * - L3: 67
 * - Mem: 419
 *
 * Zen4 (Should be, according so SOG):
 * - L1: 4
 * - L2: 14
 * - L3: 50
 * - Mem: ?
 */

#include "lib.h"
#include "log.h"
#include "mem.h"
#include "page.h"
#include "test.h"
#include <string.h>

typedef struct MemConfig MemConfig;
struct MemConfig {
    /// Total memory in B that is getting accessed
    uint64_t mem_size;
    uint64_t page_size;
    uint64_t num_iter;
    uint64_t (*measure_func)(const void *);
};

static inline uint64_t access_time2_overhead(size_t iter) {
    uint64_t sum = 0;
    for (size_t i = 0; i < iter; i++) {
        uarf_lfence();
        uarf_mfence();
        uint64_t t0 = uarf_rdpru_aperf();
        uarf_lfence();
        sum += uarf_rdpru_aperf() - t0;
        uarf_lfence();
        uarf_mfence();
    }
    return sum / iter;
}

int compare_uint64(const void *a, const void *b) {
    uint64_t arg1 = *(const uint64_t *) a;
    uint64_t arg2 = *(const uint64_t *) b;

    if (arg1 < arg2)
        return -1;
    if (arg1 > arg2)
        return 1;
    return 0;
}

UARF_TEST_CASE_ARG(basic, config) {

    const uint64_t CACHE_LINE_SIZE = 64;

    MemConfig *cfg = config;
    uarf_assert(IS_POW_TWO(cfg->mem_size));
    uarf_assert(cfg->page_size == PAGE_SIZE || cfg->page_size == PAGE_SIZE_2M);
    uarf_assert(cfg->num_iter);

    // Allocate pages
    uint64_t num_pages = div_round_up(cfg->mem_size, cfg->page_size);

    uint64_t *pages_p = uarf_malloc_or_die(num_pages * sizeof(uint64_t));

    UARF_LOG_DEBUG("Allocate %lu pages of size %luB\n", num_pages, cfg->page_size);
    for (size_t i = 0; i < num_pages; i++) {
        if (cfg->page_size == PAGE_SIZE) {
            pages_p[i] = uarf_alloc_map_or_die(cfg->page_size);
            uarf_assert(pages_p[i]);
        }
        else if (cfg->page_size == PAGE_SIZE_2M) {
            pages_p[i] = uarf_alloc_map_huge_or_die(cfg->page_size);
            uarf_assert(pages_p[i]);
        }
        else {
            uarf_assert(0);
        }
        // Ensure not demand allocated and zero-page backed
        memset(_ptr(pages_p[i]), i, cfg->page_size);
    }

    // Access data
    UARF_LOG_DEBUG("Start measurement\n");

    uint64_t num_accesses = cfg->mem_size / CACHE_LINE_SIZE;
    uarf_assert(IS_POW_TWO(num_accesses));

    uint64_t access_per_page = cfg->page_size / CACHE_LINE_SIZE;

    UARF_LOG_DEBUG("Alloc %lu results\n", cfg->num_iter * num_accesses);

    uint64_t *res = malloc(cfg->num_iter * num_accesses * sizeof(uint64_t));
    uarf_assert(res);
    size_t res_i = 0;

    for (size_t iter = 0; iter < cfg->num_iter; iter++) {
        for (size_t access = 0; access < num_accesses; access++) {
            size_t k = (access * 421 + 9) & (num_accesses - 1);
            size_t page_i = k / access_per_page;
            size_t page_offset = k - (page_i * access_per_page);
            uarf_assert(page_i < num_pages);
            uarf_assert(page_offset < access_per_page);

            res[res_i++] = cfg->measure_func(_ptr(pages_p[page_i] + page_offset));
        }
    }
    uarf_assert(res_i == cfg->num_iter * num_accesses);

    // Extract result
    qsort(res, cfg->num_iter * num_accesses, sizeof(res[0]), compare_uint64);
    uint64_t median = res[cfg->num_iter * num_accesses / 2];

    // Unmap result and pages
    uarf_free_or_die(res);

    for (size_t i = 0; i < num_pages; i++) {
        uarf_unmap_or_die(_ptr(pages_p[i]), cfg->page_size);
    }

    return median;
}

uint64_t test_sizes[22] = {1,     2,      4,      8,      16,      32,     64,    128,
                           256,   512,    1024,   2048,   4096,    8192,   16384, 32768,
                           65536, 131072, 262144, 524288, 1048576, 2097152};

MemConfig cfg1 = {.page_size = PAGE_SIZE_2M,
                  .mem_size = 0,
                  .num_iter = 10,
                  .measure_func = uarf_get_access_time2};

UARF_TEST_SUITE() {

    uarf_pi_init();

    // Disable prefetcher
    uarf_wrmsr_user(0xc0000108, 47);

    uarf_log_system_base_level = UARF_LOG_LEVEL_ERROR;

    uint64_t overhead = access_time2_overhead(1000);
    UARF_LOG_INFO("Overhead: %lu\n", overhead);
    UARF_LOG_INFO("Acceess times (minus overhead):\n");

    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); i++) {
        cfg1.mem_size = (test_sizes[i] << 10);

        uint64_t dt = 0;

        do {
            dt = UARF_TEST_RUN_CASE_ARG(basic, &cfg1);
        } while (overhead > dt);
        dt -= overhead;
        printf("%lu, %lu\n", cfg1.mem_size, dt);
    }

    uarf_pi_deinit();

    return 0;
}
