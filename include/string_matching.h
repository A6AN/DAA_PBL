#ifndef SIGNALDESK_STRING_MATCHING_H
#define SIGNALDESK_STRING_MATCHING_H
#include <stddef.h>
#include <stdint.h>
#include "common.h"

/* Byte-oriented matching: buffers may contain NUL bytes. Valid empty
   pattern matches at all tlen+1 positions, including the end. *found is
   TOTAL match count, even when max_out truncates stored positions. Caller
   owns out_positions. NULL buffer is legal only when its length is zero;
   NULL out_positions legal only when max_out is zero. Overlaps count.
   Return SD_OK or a negative status. ops may be NULL. */
int naive_search(const char *text, size_t tlen, const char *pat, size_t plen,
                 size_t *out_positions, size_t max_out, size_t *found,
                 uint64_t *ops);
int rabin_karp_search(const char *text, size_t tlen, const char *pat, size_t plen,
                      size_t *out_positions, size_t max_out, size_t *found,
                      uint64_t *ops);
int automaton_search(const char *text, size_t tlen, const char *pat, size_t plen,
                     size_t *out_positions, size_t max_out, size_t *found,
                     uint64_t *ops);
int kmp_search(const char *text, size_t tlen, const char *pat, size_t plen,
               size_t *out_positions, size_t max_out, size_t *found,
               uint64_t *ops);

/* Same search with an extra, caller-owned counter of hash collisions that
   survived the hash test but failed the direct byte comparison. No globals. */
int rabin_karp_search_stats(const char *text, size_t tlen, const char *pat,
                            size_t plen, size_t *out_positions, size_t max_out,
                            size_t *found, uint64_t *spurious_hits,
                            uint64_t *ops);
#endif
