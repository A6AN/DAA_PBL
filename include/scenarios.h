#ifndef SIGNALDESK_SCENARIOS_H
#define SIGNALDESK_SCENARIOS_H
#include <stdio.h>
/* File handles are borrowed. Paths are resolved relative to the current
   working directory or an immediately enclosing project directory. */
int newsroom_run_scenario(unsigned number, FILE *out);
#endif
