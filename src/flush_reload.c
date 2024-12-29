#include "flush_reload.h"
#include "compiler.h"
#include "lib.h"
#include "log.h"
#include "mem.h"
#include <string.h>

void fr_flush(struct FrConfig *conf) {
    LOG_TRACE("(%p)\n", conf);
    mfence();
    for (uint64_t i = 0; i < conf->num_slots; i++) {
        volatile void *p = conf->buf.p + i * FR_STRIDE;
        clflush(p);
    }
    mfence(); // Required to enforce ordering of cl flush with subsequent memory
              // operations on AMD
    sfence();
    lfence();
}

// Calculate in which bin `iteration` goes
size_t get_bin(struct FrConfig *conf, size_t iteration) {
    LOG_TRACE("(%p, %lu)\n", conf, iteration);

    for (size_t i = 0; i < conf->num_bins; i++) {
        if (iteration <= conf->bin_map[i]) {
            return i;
        }
    }

    // If no bin found, take last one
    return conf->num_bins - 1;
}

void fr_reload_binned(struct FrConfig *conf, size_t iteration) {
    LOG_TRACE("(%p, %lu)\n", conf, iteration);
    mfence();
    size_t bin_i = get_bin(conf, iteration);
    uint32_t *res_bin_p = (uint32_t *) (conf->res_p + bin_i * conf->num_slots);
    LOG_DEBUG("result address: %p\n", res_bin_p);

    // Should be completely unrolled to prevent data triggering the data cache prefetcher
#pragma GCC unroll 1024
    for (uint64_t k = 0; k < conf->num_slots; ++k) {
        size_t buf_i = (k * 13 + 9) & (conf->num_slots - 1);
        void *p = conf->buf.p + FR_STRIDE * buf_i;
        uint64_t dt = get_access_time(p);
        if (dt < conf->thresh) {
            res_bin_p[buf_i]++;
        }
    }
    mfence();
}

// Initialize the flush and reload buffer, its dummy version  and history buffer.
// Needs to be done from kernel space
struct FrConfig fr_init(uint8_t num_slots, uint8_t num_bins,
                        size_t *bin_map) {
    LOG_TRACE("(%u, %u, %p)\n", num_slots, num_bins, bin_map);

    // Assert nm_slots is power of two. Fr_reload_range only works for 2^n
    ASSERT(IS_POW_TWO(num_slots));

    ASSERT(num_bins != 0);

    struct FrConfig conf = (struct FrConfig){
        .buf = {.base_addr = FR_BUF, .addr = FR_BUF + FR_OFFSET},
        .buf2 = {.base_addr = FR_BUF2, .addr = FR_BUF2 + FR_OFFSET},
        .res_addr = FR_RES,
        .num_slots = num_slots,
        .num_bins = num_bins,
        .thresh = FR_THRESH,
        .buf_size = num_slots * FR_STRIDE + FR_OFFSET,
        .res_size = num_slots * num_bins * sizeof(uint32_t),
    };

    if (num_bins > 1) {
        memcpy(conf.bin_map, bin_map, FR_CONFIG_BIN_MAPPING_SIZE * sizeof(size_t));
    }

    map_or_die(conf.buf.base_p, conf.buf_size);
    map_or_die(conf.buf2.base_p, conf.buf_size);
    map_or_die(conf.res_p, conf.res_size);

    // Ensure it is not zero-page backed
    for (size_t i = 0; i < conf.num_slots; i++) {
        memset(conf.buf.p + i * FR_STRIDE, '0' + i, FR_STRIDE);
    }

    // Assign FR_BUF[i] = i + 1
    // Used to create dependant memory accesses
    for (size_t i = 0; i < conf.num_slots; i++) {
        *(uint64_t *) (conf.buf.p + (i * FR_STRIDE)) = i + 1;
    }

    return conf;
}

void fr_deinit(struct FrConfig *conf) {
    LOG_TRACE("(%p)\n", conf);

    unmap_or_die(conf->buf.base_p, conf->buf_size);
    unmap_or_die(conf->buf2.base_p, conf->buf_size);
    unmap_or_die(conf->res_p, conf->res_size);
}

void fr_print(struct FrConfig *conf) {
    LOG_TRACE("(%p)\n", conf);

    if (conf->num_bins == 1) {
        for (size_t i = 0; i < conf->num_slots; i++) {
            printf("%s%04u " LOG_C_RESET, conf->res_p[i] ? LOG_C_DARK_RED : "",
                   conf->res_p[i]);
        }
        printf("\n");
        return;
    }

    // Calculate max possible number of elements per bin
    uint64_t max_bin[conf->num_bins];
    max_bin[0] = conf->bin_map[0] + 1;
    for (size_t i = 1; i < conf->num_bins; i++) {
        max_bin[i] = conf->bin_map[i] - conf->bin_map[i - 1];
    }

    uint64_t total[conf->num_slots];
    memset(total, 0, sizeof(total));

    for (uint8_t bin = 0; bin < conf->num_bins; bin++) {
        if (bin < conf->num_bins - 1) {
            printf("%5lu (%4lu): ", conf->bin_map[bin], max_bin[bin]);
        }
        else {
            printf("   remainder: ");
        }
        for (size_t slot = 0; slot < conf->num_slots; slot++) {
            uint32_t hits = *(conf->res_p + bin * conf->num_slots + slot);
            total[slot] += hits;
            printf("%s%04u " LOG_C_RESET, hits ? LOG_C_DARK_RED : "", hits);
        }
        printf("\n");
    }

    printf("-------------");
    for (size_t i = 0; i < conf->num_slots; i++) {
        printf("-----");
    }
    printf("\n");

    // Print total sum
    printf("       Total: ");
    for (size_t i = 0; i < conf->num_slots; i++) {
        printf("%s%04ld " LOG_C_RESET, total[i] ? LOG_C_DARK_RED : "", total[i]);
    }
    printf("\n");

    printf("=============");
    for (size_t i = 0; i < conf->num_slots; i++) {
        printf("=====");
    }
    printf("\n");
}
