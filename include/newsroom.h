#ifndef SIGNALDESK_NEWSROOM_H
#define SIGNALDESK_NEWSROOM_H
#include "article_manager.h"
#include "greedy.h"
#include "dynamic_programming.h"
#include "graph_algorithms.h"
#include "divide_conquer.h"
#include <stddef.h>
#include <stdint.h>

#define SD_DUPLICATE_THRESHOLD 0.72
/* Urgency = base_score * (1 / (1 + hours_old/24))^2 +
   4 * window_article_count. The time-decay power is evaluated by the
   existing fast_power_double implementation. This is a scheduling priority,
   not a measure of factual accuracy. */
#define SD_URGENCY_DECAY_EXPONENT (-2LL)

typedef struct {
    size_t articles, sources, events, candidate_pairs, awaiting_verification;
    size_t high_urgency, available_capacity;
} DashboardStats;
typedef struct {
    int article_a, article_b;
    double score;
    char passage[MAX_HEADLINE]; /* matching headline token passage */
} OverlapPair;
typedef struct {
    int event_id;
    long long start_time;
    size_t articles;
} CoverageWindow;
typedef struct {
    int article_id, checker_id; /* checker_id == -1 means unassigned */
} Assignment;
typedef struct {
    Assignment *items; /* caller-owned */
    size_t count, assigned, unassigned;
    size_t *workloads; /* caller-owned array of checker_count */
    size_t checker_count;
} AllocationReport;

typedef struct {
    size_t before_assigned, after_assigned, before_unassigned, after_unassigned;
} ShockComparison;

/* All returned heap buffers belong to caller, freed with free() unless noted. */
int newsroom_dashboard(const ArticleStore *articles, const SourceTable *sources,
                       const EventTable *events, const CheckerTable *checkers,
                       DashboardStats *stats);
int newsroom_overlaps(const ArticleStore *store, OverlapPair **pairs,
                      size_t *count, uint64_t *ops);
int newsroom_timeline(const ArticleStore *store, int event_id, Article ***timeline,
                      size_t *count, uint64_t *ops);
/* Grouping by tag: matching article pointers sorted chronologically. */
int newsroom_tag_timeline(const ArticleStore *store, const char *tag,
                          Article ***timeline, size_t *count, uint64_t *ops);
/* Windows use [origin + k*width, origin +(k+1)*width). Caller supplies origin.
   Negative timestamps are supported. Output covers windows 0..window_count-1. */
int newsroom_coverage(const ArticleStore *store, int event_id, long long origin,
                      long long width, size_t window_count, size_t *counts);
/* Maximum subarray over differences (counts[i+1]-counts[i]); if only one
   window, returns index 0 and delta 0. Both indices refer to delta windows. */
int newsroom_surge(const size_t *counts, size_t n, size_t *first,
                   size_t *last, long long *delta, uint64_t *ops);
int newsroom_urgency(double base_score, long long age_hours,
                     size_t coverage, double *score, uint64_t *ops);
/* Matching determines candidate compatibility; capacity flow determines the
   maximum feasible workload count. Assignments are then extracted directly
   from positive flow on task-to-checker arcs. Tasks consume one unit, and
   checker capacity is available_capacity - assigned_workload. */
int newsroom_allocate(const FactCheckTask *tasks, size_t task_count,
                      const FactChecker *checkers, size_t checker_count,
                      AllocationReport *report, uint64_t *ops);
void newsroom_allocation_free(AllocationReport *report);
int newsroom_compare_shock(const FactCheckTask *before, size_t before_count,
                           const FactCheckTask *after, size_t after_count,
                           const FactChecker *checkers, size_t checker_count,
                           ShockComparison *result, uint64_t *ops);
/* Source-reference matrix is n*n bytes; references[u*n+v] means u cites v. */
int newsroom_source_celebrity(const unsigned char *references, size_t n,
                             ptrdiff_t *index, uint64_t *ops);
int newsroom_network_paths(const GraphList *graph, size_t origin,
                           double *distances, size_t *predecessors, uint64_t *ops);
int newsroom_network_mst(const GraphMatrix *graph, int use_kruskal,
                         SDTreeEdge *edges, size_t *count, double *weight,
                         uint64_t *ops);
int newsroom_network_all_pairs(const GraphMatrix *graph, double **distance,
                               size_t **next_hop, int *negative_cycle,
                               uint64_t *ops);
#endif
