#ifndef SIGNALDESK_ARTICLE_MANAGER_H
#define SIGNALDESK_ARTICLE_MANAGER_H
#include "models.h"
#include "sorting_searching.h"
#include "common.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    Article *items; /* owned, freed with article_store_free */
    size_t count, capacity;
} ArticleStore;
typedef struct { size_t loaded, rejected, first_error_line; } CSVReport;
typedef struct { Source *items; size_t count; } SourceTable;
typedef struct { Event *items; size_t count; } EventTable;
typedef struct { FactChecker *items; size_t count; } CheckerTable;

void article_store_init(ArticleStore *store);
void article_store_free(ArticleStore *store);
int article_store_add(ArticleStore *store, const Article *article);
/* Indexed lookup delegates the search itself to Phase 1's binary search.
   Result is a borrowed pointer, invalidated by a later add or free. */
int article_store_find(const ArticleStore *store, int id, const Article **result, uint64_t *ops);
/* Returns caller-owned array of borrowed pointers; caller frees array. */
int article_store_sort(const ArticleStore *store, ArticleSortKey key, int quick,
                       Article ***ordered, uint64_t *ops);
/* Returns caller-owned article IDs, including each article at most once. */
int article_store_phrase(const ArticleStore *store, const char *phrase,
                         int (*matcher)(const char *,size_t,const char *,size_t,
                                        size_t *,size_t,size_t *,uint64_t *),
                         int **ids, size_t *count, uint64_t *ops);
/* CSV columns documented by the headers in sample data. Input supports escaped
   quotes, commas and embedded newlines. Malformed records are counted and
   skipped. Failed I/O or allocation returns an error; valid records remain. */
int article_csv_read(FILE *file, ArticleStore *store, CSVReport *report);
int article_csv_write(FILE *file, const ArticleStore *store);
int sources_csv_read(FILE *file, SourceTable *table, CSVReport *report);
int sources_csv_write(FILE *file, const SourceTable *table);
int events_csv_read(FILE *file, EventTable *table, CSVReport *report);
int events_csv_write(FILE *file, const EventTable *table);
int checkers_csv_read(FILE *file, CheckerTable *table, CSVReport *report);
int checkers_csv_write(FILE *file, const CheckerTable *table);
void sources_free(SourceTable *table);
void events_free(EventTable *table);
void checkers_free(CheckerTable *table);
#endif
