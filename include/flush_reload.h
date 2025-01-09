/**
 * Flush+Reload tools.
 */

// L3 miss
#define FR_THRESH 150

// Address of first flush and reload buffer
#define FR_BUF 0xfff1f200000ul

// Address of second flush and reload buffer
#define FR_BUF2 0xfff2f200000ul

// Address of result array
#define FR_RES 0xfff3f200000ul

// Distance between two entries in the buffer
#define FR_STRIDE_BITS 12
#define FR_STRIDE      (1UL << FR_STRIDE_BITS)

#define FR_OFFSET 0x180

#ifndef __ASSEMBLY__
#include "compiler.h"
#include "lib.h"
#include "log.h"
#include <string.h>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG LOG_TAG_FR
#endif

#define FR_CONFIG_BIN_MAPPING_SIZE 10
struct FrConfig {
    // First flush and reload buffer
    struct {
        // Page Aligned address
        union {
            char *base_p;
            uintptr_t base_addr;
        };
        // At some offset
        union {
            char *p;
            uintptr_t addr;
        };
    } buf;
    // Second flush and reload buffer
    struct {
        // Page Aligned address
        union {
            char *base_p;
            uintptr_t base_addr;
        };
        // At some offset
        union {
            char *p;
            uintptr_t addr;
        };
    } buf2;
    // Address of results array
    // [[bin 0 for all num_slots entries], [bin 1 for all num_slots entries], ... [bin
    // num_bins - 1 for all num_slots entries]]
    union {
        uint32_t *res_p;
        uintptr_t res_addr;
    };
    // Number of slots (entries) in the buffer
    uint8_t num_slots;
    // Number of bins to distribute the
    uint8_t num_bins;
    // Cache hit/miss threshold
    uint16_t thresh;
    // Mapping of iterations to bins
    // Having this as a variable sized array leads to weird bugs when accessing the array
    size_t bin_map[FR_CONFIG_BIN_MAPPING_SIZE];
    // Size of buffer and result array in bytes
    size_t buf_size;
    size_t res_size;
};

static __always_inline void fr_reset(struct FrConfig *conf) {
    LOG_TRACE("(%p)\n", conf);
    memset(conf->res_p, 0, conf->num_slots * conf->num_bins * sizeof(uint32_t));
}

void fr_flush(struct FrConfig *conf);
void fr_reload_binned(struct FrConfig *conf, size_t iteration);

struct FrConfig fr_init(uint8_t num_slots, uint8_t num_bins, size_t *bin_map);

void fr_deinit(struct FrConfig *conf);
void fr_print(struct FrConfig *conf);

static __always_inline void fr_reload(struct FrConfig *conf) {
    LOG_TRACE("(%p)\n", conf);
    fr_reload_binned(conf, 0);
}

#endif /* __ASSEMBLY__ */
