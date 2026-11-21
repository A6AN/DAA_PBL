#ifndef SIGNALDESK_TEST_HARNESS_H
#define SIGNALDESK_TEST_HARNESS_H

#include <stdio.h>
#include <string.h>

/* Each test executable owns its own counters. */
static int harness_passed = 0;
static int harness_failed = 0;

#define ASSERT_TRUE(expr) do { \
    if (expr) { ++harness_passed; } \
    else { ++harness_failed; fprintf(stderr, "%s:%d: ASSERT_TRUE failed: %s\n", \
        __FILE__, __LINE__, #expr); } \
} while (0)

#define ASSERT_EQ_INT(expected, actual) do { \
    long long sd_expected = (long long)(expected); \
    long long sd_actual = (long long)(actual); \
    if (sd_expected == sd_actual) { ++harness_passed; } \
    else { ++harness_failed; fprintf(stderr, \
        "%s:%d: ASSERT_EQ_INT failed: expected %lld, got %lld\n", \
        __FILE__, __LINE__, sd_expected, sd_actual); } \
} while (0)

#define ASSERT_EQ_STR(expected, actual) do { \
    const char *sd_expected = (expected); \
    const char *sd_actual = (actual); \
    if (sd_expected != NULL && sd_actual != NULL && strcmp(sd_expected, sd_actual) == 0) { \
        ++harness_passed; \
    } else { ++harness_failed; fprintf(stderr, \
        "%s:%d: ASSERT_EQ_STR failed\n", __FILE__, __LINE__); } \
} while (0)

/* Absolute tolerance; rejects NaN and negative tolerances. */
#define ASSERT_EQ_DOUBLE(expected, actual, tolerance) do { \
    double sd_expected = (double)(expected); \
    double sd_actual = (double)(actual); \
    double sd_tolerance = (double)(tolerance); \
    double sd_delta = sd_expected - sd_actual; \
    if (sd_delta < 0.0) sd_delta = -sd_delta; \
    if (sd_tolerance >= 0.0 && sd_delta <= sd_tolerance) { ++harness_passed; } \
    else { ++harness_failed; fprintf(stderr, \
        "%s:%d: ASSERT_EQ_DOUBLE failed: expected %.17g, got %.17g\n", \
        __FILE__, __LINE__, sd_expected, sd_actual); } \
} while (0)

static inline int harness_summary(void) {
    printf("Passed: %d | Failed: %d\n", harness_passed, harness_failed);
    return harness_failed == 0 ? 0 : 1;
}

#endif
