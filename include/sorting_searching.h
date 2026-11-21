#ifndef SIGNALDESK_SORTING_SEARCHING_H
#define SIGNALDESK_SORTING_SEARCHING_H
#include <stddef.h>
#include <stdint.h>
#include "common.h"
#include "models.h"

typedef enum {
    ARTICLE_BY_ID = 0,
    ARTICLE_BY_PUBLICATION_TIME = 1,
    ARTICLE_BY_URGENCY = 2
} ArticleSortKey;

/* All sorts are ascending, modify the caller-owned array and allocate no
   caller-owned results. n=0 allows NULL; n>0 requires non-NULL.
   Article sorts reorder pointers, not the Article records themselves.
   Article key ties use ID, then original order for merge sort. */
int merge_sort(int *values, size_t n, uint64_t *ops);
int quick_sort(int *values, size_t n, uint64_t *ops);
int merge_sort_articles(Article **items, size_t n, ArticleSortKey key, uint64_t *ops);
int quick_sort_articles(Article **items, size_t n, ArticleSortKey key, uint64_t *ops);

/* Sorted ascending input. On success, *index is first matching offset or -1.
   Empty array is valid. Indices must fit ptrdiff_t. */
int binary_search_iterative(const int *values, size_t n, int target,
                            ptrdiff_t *index, uint64_t *ops);
int binary_search_recursive(const int *values, size_t n, int target,
                            ptrdiff_t *index, uint64_t *ops);
#endif
