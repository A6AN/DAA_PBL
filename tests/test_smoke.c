#include "test_harness.h"
#include "common.h"
#include "models.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void test_status_and_models(void) {
    ASSERT_EQ_INT(0, SD_OK);
    ASSERT_TRUE(SD_ERR_INVALID_ARGUMENT < 0);
    ASSERT_TRUE(SD_ERR_OUT_OF_MEMORY < 0);
    ASSERT_TRUE(SD_ERR_NOT_FOUND < 0);
    ASSERT_TRUE(SD_ERR_INFEASIBLE < 0);
    ASSERT_TRUE(SD_ERR_NEGATIVE_WEIGHT < 0);
    ASSERT_TRUE(SD_ERR_NEGATIVE_CYCLE < 0);
    ASSERT_TRUE(SD_ERR_OVERFLOW < 0);
    ASSERT_EQ_INT(256, MAX_HEADLINE);
    ASSERT_EQ_INT(8192, MAX_BODY);
    ASSERT_EQ_INT(10, MAX_TAGS);
    ASSERT_EQ_INT(VERIFY_PENDING, 0);
    ASSERT_EQ_INT(URGENCY_CRITICAL, 3);
    ASSERT_TRUE(sizeof(Article) > sizeof(Source));
    ASSERT_TRUE(SIZE_SMALL < SIZE_MEDIUM && SIZE_MEDIUM < SIZE_LARGE);
}

static void test_strings(void) {
    char text[5];
    ASSERT_EQ_INT(0, safe_strncpy(text, "cat", sizeof(text)));
    ASSERT_EQ_STR("cat", text);
    ASSERT_EQ_INT(1, safe_strncpy(text, "elephant", sizeof(text)));
    ASSERT_EQ_STR("elep", text);
    ASSERT_EQ_INT(0, safe_strncpy(text, "", sizeof(text)));
    ASSERT_EQ_STR("", text);
    ASSERT_EQ_INT(1, safe_strncpy(text, "x", 0));
    ASSERT_EQ_INT(1, safe_strncpy(NULL, "x", 5));
    ASSERT_EQ_INT(1, safe_strncpy(text, NULL, sizeof(text)));
}

static void test_memory(void) {
    unsigned char *data = checked_malloc(4);
    ASSERT_TRUE(data != NULL);
    if (data == NULL) return;
    data[0] = 0x11;
    data[1] = 0x22;
    data[2] = 0x33;
    data[3] = 0x44;
    unsigned char *grown = checked_realloc(data, 16);
    ASSERT_TRUE(grown != NULL);
    if (grown == NULL) {
        free(data);
        return;
    }
    data = grown;
    ASSERT_EQ_INT(0x11, data[0]);
    ASSERT_EQ_INT(0x22, data[1]);
    ASSERT_EQ_INT(0x33, data[2]);
    ASSERT_EQ_INT(0x44, data[3]);
    memset(data + 4, 0x55, 12);
    ASSERT_EQ_INT(0x55, data[15]);
    free(data);

    int *zeroed = checked_calloc(8, sizeof(*zeroed));
    ASSERT_TRUE(zeroed != NULL);
    if (zeroed != NULL) {
        for (int i = 0; i < 8; ++i) ASSERT_EQ_INT(0, zeroed[i]);
        free(zeroed);
    }
    ASSERT_TRUE(checked_calloc(SIZE_MAX, 2) == NULL);
    void *empty = checked_malloc(0);
    ASSERT_TRUE(empty != NULL);
    free(empty);
}

static void test_lcg(void) {
    uint64_t first[5];
    lcg_seed(UINT64_C(42));
    for (size_t i = 0; i < 5; ++i) first[i] = lcg_next();
    lcg_seed(UINT64_C(42));
    for (size_t i = 0; i < 5; ++i) ASSERT_TRUE(first[i] == lcg_next());

    uint64_t reference = UINT64_C(42);
    for (size_t i = 0; i < 5; ++i) {
        reference = reference * UINT64_C(6364136223846793005) +
                    UINT64_C(1442695040888963407);
        ASSERT_TRUE(first[i] == reference);
    }
    for (size_t i = 0; i < 100; ++i) {
        uint64_t value = lcg_range(10, 20);
        ASSERT_TRUE(value >= 10 && value <= 20);
    }
    ASSERT_TRUE(lcg_range(7, 7) == 7);
    ASSERT_TRUE(lcg_range(9, 3) == 9);
    (void)lcg_range(0, UINT64_MAX);
}

static void test_timing_and_assertions(void) {
    ASSERT_EQ_DOUBLE(1.0, 1.00001, 0.0001);
    ASSERT_TRUE(sd_time_ns() != 0);
}

int main(void) {
    test_status_and_models();
    test_strings();
    test_memory();
    test_lcg();
    test_timing_and_assertions();
    return harness_summary();
}
