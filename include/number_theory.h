#ifndef SIGNALDESK_NUMBER_THEORY_H
#define SIGNALDESK_NUMBER_THEORY_H
#include <stddef.h>
#include <stdint.h>
#include "common.h"

/* Moduli must be positive and pairwise coprime. Computes the unique value
   in [0, product) as well as the product; overflow is an error. n=0
   represents the unique empty system: value=0, product=1. */
int crt_solve(const uint64_t *remainders, const uint64_t *moduli, size_t n,
              uint64_t *value, uint64_t *product, uint64_t *ops);
/* Reconstructs an ingestion counter from partition-wise residues; the
   counter must be known to lie below the product of all moduli. */
int reconstruct_ingestion_counter(const uint64_t *partition_remainders,
                                  const uint64_t *partition_moduli,
                                  size_t partitions, uint64_t *counter,
                                  uint64_t *range, uint64_t *ops);
#endif
