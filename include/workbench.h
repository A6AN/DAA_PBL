#ifndef SIGNALDESK_WORKBENCH_H
#define SIGNALDESK_WORKBENCH_H
#include <stddef.h>
#include <stdio.h>
/* The metadata table and sample dispatch live together in workbench.c.
   Streams are borrowed; caller retains ownership. */
size_t workbench_count(void);
int workbench_list(FILE *out);
int workbench_run(size_t index, FILE *out);
int workbench_run_all(FILE *out);
#endif
