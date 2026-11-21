#include "sorting_searching.h"
#include "test_harness.h"
#include <stdlib.h>
#include <string.h>

static int oracle_int(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}
static ArticleSortKey oracle_key;
static int oracle_article(const void *pa, const void *pb) {
    const Article *a = *(Article *const *)pa, *b = *(Article *const *)pb;
    if (oracle_key == ARTICLE_BY_PUBLICATION_TIME && a->publication_time != b->publication_time)
        return (a->publication_time > b->publication_time) -
               (a->publication_time < b->publication_time);
    if (oracle_key == ARTICLE_BY_URGENCY && a->urgency_score != b->urgency_score)
        return (a->urgency_score > b->urgency_score) -
               (a->urgency_score < b->urgency_score);
    return (a->id > b->id) - (a->id < b->id);
}
static void check_int_case(const int *input, size_t n) {
    int a[130], b[130], reference[130];
    for (size_t i = 0; i < n; ++i) a[i] = b[i] = reference[i] = input[i];
    qsort(reference, n, sizeof(int), oracle_int);
    uint64_t ops = 0;
    ASSERT_EQ_INT(SD_OK, merge_sort(a, n, &ops));
    ASSERT_EQ_INT(SD_OK, quick_sort(b, n, &ops));
    for (size_t i = 0; i < n; ++i) {
        ASSERT_EQ_INT(reference[i], a[i]);
        ASSERT_EQ_INT(reference[i], b[i]);
    }
    for (int target = -20; target <= 20; ++target) {
        ptrdiff_t expected = -1, iterative = -2, recursive = -2;
        for (size_t i = 0; i < n; ++i)
            if (reference[i] == target) { expected = (ptrdiff_t)i; break; }
        ASSERT_EQ_INT(SD_OK, binary_search_iterative(a,n,target,&iterative,&ops));
        ASSERT_EQ_INT(SD_OK, binary_search_recursive(a,n,target,&recursive,NULL));
        ASSERT_EQ_INT(expected, iterative);
        ASSERT_EQ_INT(expected, recursive);
    }
}
static void check_articles(void) {
    Article records[32] = {{0}};
    Article *a[32], *b[32], *ref[32];
    for (int trial = 0; trial < 200; ++trial) {
        size_t n = (size_t)lcg_range(0,32);
        for (size_t i = 0; i < n; ++i) {
            records[i].id = (int)i;
            records[i].publication_time = (long long)lcg_range(0,9);
            records[i].urgency_score = (double)lcg_range(0,9);
            a[i] = b[i] = ref[i] = &records[i];
        }
        for (int key = ARTICLE_BY_ID; key <= ARTICLE_BY_URGENCY; ++key) {
            oracle_key = (ArticleSortKey)key;
            qsort(ref,n,sizeof(*ref),oracle_article);
            ASSERT_EQ_INT(SD_OK, merge_sort_articles(a,n,oracle_key,NULL));
            ASSERT_EQ_INT(SD_OK, quick_sort_articles(b,n,oracle_key,NULL));
            for (size_t i = 0; i < n; ++i) {
                ASSERT_TRUE(ref[i] == a[i]);
                ASSERT_TRUE(ref[i] == b[i]);
            }
        }
    }
}
int main(void) {
    lcg_seed(20261121);
    check_int_case(NULL,0);
    int one[] = {4}, sorted[] = {-3,-2,-1,0,1,2},
        reversed[] = {5,4,3,2,1,0}, same[] = {7,7,7,7,7},
        duplicates[] = {3,1,3,2,1,2,3,1};
    check_int_case(one,1);
    check_int_case(sorted,6);
    check_int_case(reversed,6);
    check_int_case(same,5);
    check_int_case(duplicates,8);
    for (int trial = 0; trial < 200; ++trial) {
        int values[130];
        size_t n = (size_t)lcg_range(0,128);
        for (size_t i = 0; i < n; ++i)
            values[i] = (int)lcg_range(0,40)-20;
        check_int_case(values,n);
    }
    check_articles();
    ptrdiff_t found = 99;
    ASSERT_EQ_INT(SD_OK, binary_search_iterative(NULL,0,1,&found,NULL));
    ASSERT_EQ_INT(-1,found);
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,merge_sort(NULL,2,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,quick_sort(NULL,2,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,binary_search_iterative(NULL,1,1,&found,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,binary_search_recursive(NULL,1,1,&found,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,binary_search_recursive(one,1,1,NULL,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,merge_sort_articles(NULL,1,ARTICLE_BY_ID,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,quick_sort_articles(NULL,1,ARTICLE_BY_ID,NULL));
    ASSERT_EQ_INT(SD_ERR_INVALID_ARGUMENT,merge_sort_articles(NULL,0,(ArticleSortKey)-1,NULL));
    return harness_summary();
}
