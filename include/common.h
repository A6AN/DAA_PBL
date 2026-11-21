#ifndef SIGNALDESK_COMMON_H
#define SIGNALDESK_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>

/* Every algorithm may accept uint64_t *ops; NULL disables counting. */
typedef enum {
    SD_OK = 0,
    SD_ERR_INVALID_ARGUMENT = -1,
    SD_ERR_OUT_OF_MEMORY = -2,
    SD_ERR_NOT_FOUND = -3,
    SD_ERR_INFEASIBLE = -4,
    SD_ERR_NEGATIVE_WEIGHT = -5,
    SD_ERR_NEGATIVE_CYCLE = -6,
    SD_ERR_OVERFLOW = -7
} SDStatus;

#define SIZE_SMALL 100u
#define SIZE_MEDIUM 1000u
#define SIZE_LARGE 10000u

/* Copies into dst and NUL-terminates when capacity > 0.
   Returns 1 on truncation or invalid input; 0 on a complete copy.
   Source and destination must not overlap. */
static inline int safe_strncpy(char *dst, const char *src, size_t capacity) {
    if (dst == NULL || src == NULL || capacity == 0) {
        return 1;
    }
    size_t i = 0;
    while (i + 1 < capacity && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
    return src[i] != '\0';
}

/* Returned blocks belong to the caller and must be freed. */
static inline void *checked_malloc(size_t bytes) {
    return malloc(bytes == 0 ? 1 : bytes);
}

static inline void *checked_calloc(size_t count, size_t bytes) {
    if (bytes != 0 && count > SIZE_MAX / bytes) {
        return NULL;
    }
    return calloc(count == 0 ? 1 : count, bytes == 0 ? 1 : bytes);
}

/* On failure, original stays allocated and remains owned by caller.
   On success, the returned pointer replaces the original pointer. */
static inline void *checked_realloc(void *block, size_t bytes) {
    return realloc(block, bytes == 0 ? 1 : bytes);
}

/* LCG state is private to each translation unit using this header.
   Re-seed before each reproducible dataset or test sequence. */
static inline uint64_t *sd_lcg_state(void) {
    static uint64_t state = UINT64_C(1);
    return &state;
}

static inline void lcg_seed(uint64_t seed) {
    *sd_lcg_state() = seed;
}

static inline uint64_t lcg_next(void) {
    uint64_t *state = sd_lcg_state();
    *state = (*state * UINT64_C(6364136223846793005)) +
             UINT64_C(1442695040888963407);
    return *state;
}

/* Inclusive bounds; invalid ranges return lo. Full-width ranges are valid.
   Rejection sampling avoids modulo bias for other ranges. */
static inline uint64_t lcg_range(uint64_t lo, uint64_t hi) {
    if (lo >= hi) {
        return lo;
    }
    uint64_t span = hi - lo + UINT64_C(1);
    if (span == 0) {
        return lcg_next();
    }
    uint64_t threshold = (uint64_t)(-span) % span;
    uint64_t value;
    do {
        value = lcg_next();
    } while (value < threshold);
    return lo + value % span;
}

/* Nanoseconds since the C11 TIME_UTC epoch; 0 indicates clock failure.
   Benchmark elapsed time is computed from the difference of two samples. */
static inline uint64_t sd_time_ns(void) {
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) != TIME_UTC || ts.tv_sec < 0) {
        return 0;
    }
    uint64_t sec = (uint64_t)ts.tv_sec;
    if (sec > (UINT64_MAX - (uint64_t)ts.tv_nsec) / UINT64_C(1000000000)) {
        return 0;
    }
    return sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

#endif
