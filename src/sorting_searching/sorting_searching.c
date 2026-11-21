#include "sorting_searching.h"
#include <stdint.h>
#include <stdlib.h>

static int valid_key(ArticleSortKey key) {
    return key == ARTICLE_BY_ID || key == ARTICLE_BY_PUBLICATION_TIME ||
           key == ARTICLE_BY_URGENCY;
}
static int article_compare(const Article *a, const Article *b, ArticleSortKey key,
                           uint64_t *ops) {
    if (ops) ++*ops;
    if (key == ARTICLE_BY_PUBLICATION_TIME && a->publication_time != b->publication_time)
        return (a->publication_time > b->publication_time) -
               (a->publication_time < b->publication_time);
    if (key == ARTICLE_BY_URGENCY && a->urgency_score != b->urgency_score)
        return (a->urgency_score > b->urgency_score) -
               (a->urgency_score < b->urgency_score);
    return (a->id > b->id) - (a->id < b->id);
}
static int int_compare(int a, int b, uint64_t *ops) {
    if (ops) ++*ops;
    return (a > b) - (a < b);
}
static void merge_int(int *a, int *tmp, size_t lo, size_t mid, size_t hi,
                      uint64_t *ops) {
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi)
        tmp[k++] = int_compare(a[i], a[j], ops) <= 0 ? a[i++] : a[j++];
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi) tmp[k++] = a[j++];
    for (k = lo; k < hi; ++k) a[k] = tmp[k];
}
static void merge_int_rec(int *a, int *tmp, size_t lo, size_t hi, uint64_t *ops) {
    if (hi - lo < 2) return;
    size_t mid = lo + (hi - lo) / 2;
    merge_int_rec(a, tmp, lo, mid, ops);
    merge_int_rec(a, tmp, mid, hi, ops);
    merge_int(a, tmp, lo, mid, hi, ops);
}
int merge_sort(int *a, size_t n, uint64_t *ops) {
    if ((!a && n) || n > SIZE_MAX / sizeof(*a)) return SD_ERR_INVALID_ARGUMENT;
    if (n < 2) return SD_OK;
    int *tmp = checked_malloc(n * sizeof(*tmp));
    if (!tmp) return SD_ERR_OUT_OF_MEMORY;
    merge_int_rec(a, tmp, 0, n, ops);
    free(tmp);
    return SD_OK;
}
static void merge_articles_rec(Article **a, Article **tmp, size_t lo,
                               size_t hi, ArticleSortKey key, uint64_t *ops) {
    if (hi - lo < 2) return;
    size_t mid = lo + (hi - lo) / 2;
    merge_articles_rec(a, tmp, lo, mid, key, ops);
    merge_articles_rec(a, tmp, mid, hi, key, ops);
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi)
        tmp[k++] = article_compare(a[i], a[j], key, ops) <= 0 ? a[i++] : a[j++];
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi) tmp[k++] = a[j++];
    for (k = lo; k < hi; ++k) a[k] = tmp[k];
}
static int valid_articles(Article *const *a, size_t n, ArticleSortKey key) {
    if (!valid_key(key) || (!a && n) || n > SIZE_MAX / sizeof(*a)) return 0;
    for (size_t i = 0; i < n; ++i)
        if (!a[i] || (key == ARTICLE_BY_URGENCY &&
            !(a[i]->urgency_score == a[i]->urgency_score))) return 0;
    return 1;
}
int merge_sort_articles(Article **a, size_t n, ArticleSortKey key, uint64_t *ops) {
    if (!valid_articles(a, n, key)) return SD_ERR_INVALID_ARGUMENT;
    if (n < 2) return SD_OK;
    Article **tmp = checked_malloc(n * sizeof(*tmp));
    if (!tmp) return SD_ERR_OUT_OF_MEMORY;
    merge_articles_rec(a, tmp, 0, n, key, ops);
    free(tmp);
    return SD_OK;
}
static void swap_int(int *a, int *b) { int x = *a; *a = *b; *b = x; }
static void swap_ptr(Article **a, Article **b) { Article *x = *a; *a = *b; *b = x; }
static size_t median_int(int *a, size_t i, size_t j, size_t k, uint64_t *ops) {
    if (int_compare(a[i], a[j], ops) < 0)
        return int_compare(a[j], a[k], ops) < 0 ? j :
               (int_compare(a[i], a[k], ops) < 0 ? k : i);
    return int_compare(a[i], a[k], ops) < 0 ? i :
           (int_compare(a[j], a[k], ops) < 0 ? k : j);
}
/* Three-way partition keeps equal-valued inputs linear, and recursion always
   visits the smaller side first, bounding call-stack depth. */
static void quick_int(int *a, size_t lo, size_t hi, uint64_t *ops) {
    while (hi - lo > 1) {
        int pivot = a[median_int(a, lo, lo + (hi - lo) / 2, hi - 1, ops)];
        size_t lt = lo, i = lo, gt = hi;
        while (i < gt) {
            int c = int_compare(a[i], pivot, ops);
            if (c < 0) swap_int(&a[lt++], &a[i++]);
            else if (c > 0) swap_int(&a[i], &a[--gt]);
            else ++i;
        }
        if (lt - lo < hi - gt) { quick_int(a, lo, lt, ops); lo = gt; }
        else { quick_int(a, gt, hi, ops); hi = lt; }
    }
}
int quick_sort(int *a, size_t n, uint64_t *ops) {
    if ((!a && n) || n > PTRDIFF_MAX / sizeof(*a)) return SD_ERR_INVALID_ARGUMENT;
    quick_int(a, 0, n, ops);
    return SD_OK;
}
static size_t median_article(Article **a, size_t i, size_t j, size_t k,
                             ArticleSortKey key, uint64_t *ops) {
    if (article_compare(a[i], a[j], key, ops) < 0)
        return article_compare(a[j], a[k], key, ops) < 0 ? j :
               (article_compare(a[i], a[k], key, ops) < 0 ? k : i);
    return article_compare(a[i], a[k], key, ops) < 0 ? i :
           (article_compare(a[j], a[k], key, ops) < 0 ? k : j);
}
static void quick_articles(Article **a, size_t lo, size_t hi,
                           ArticleSortKey key, uint64_t *ops) {
    while (hi - lo > 1) {
        Article *pivot = a[median_article(a, lo, lo + (hi - lo) / 2,
                                           hi - 1, key, ops)];
        size_t lt = lo, i = lo, gt = hi;
        while (i < gt) {
            int c = article_compare(a[i], pivot, key, ops);
            if (c < 0) swap_ptr(&a[lt++], &a[i++]);
            else if (c > 0) swap_ptr(&a[i], &a[--gt]);
            else ++i;
        }
        if (lt - lo < hi - gt) { quick_articles(a, lo, lt, key, ops); lo = gt; }
        else { quick_articles(a, gt, hi, key, ops); hi = lt; }
    }
}
int quick_sort_articles(Article **a, size_t n, ArticleSortKey key, uint64_t *ops) {
    if (!valid_articles(a, n, key) || n > PTRDIFF_MAX / sizeof(*a))
        return SD_ERR_INVALID_ARGUMENT;
    quick_articles(a, 0, n, key, ops);
    return SD_OK;
}
int binary_search_iterative(const int *a, size_t n, int target,
                            ptrdiff_t *index, uint64_t *ops) {
    if (!index || (!a && n) || n > PTRDIFF_MAX) return SD_ERR_INVALID_ARGUMENT;
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (int_compare(a[mid], target, ops) < 0) lo = mid + 1;
        else hi = mid;
    }
    *index = lo < n && int_compare(a[lo], target, ops) == 0 ? (ptrdiff_t)lo : -1;
    return SD_OK;
}
static size_t lower_recursive(const int *a, size_t lo, size_t hi,
                              int target, uint64_t *ops) {
    if (lo == hi) return lo;
    size_t mid = lo + (hi - lo) / 2;
    if (int_compare(a[mid], target, ops) < 0)
        return lower_recursive(a, mid + 1, hi, target, ops);
    return lower_recursive(a, lo, mid, target, ops);
}
int binary_search_recursive(const int *a, size_t n, int target,
                            ptrdiff_t *index, uint64_t *ops) {
    if (!index || (!a && n) || n > PTRDIFF_MAX) return SD_ERR_INVALID_ARGUMENT;
    size_t first = lower_recursive(a, 0, n, target, ops);
    *index = first < n && int_compare(a[first], target, ops) == 0 ?
             (ptrdiff_t)first : -1;
    return SD_OK;
}
