
#ifndef DEBUG_H
#define DEBUG_H 1

#include <stdio.h>

/**
 * Debug type.
 */

typedef struct {
  const char *name;
  int color;
  int enabled;
  FILE *stream;
} debug_t;

/**
 * Output a debugging message from the given `debugger`.
 */

void
debug(debug_t *debugger, const char *fmt, ...);

/**
 * Check if a debugger is enabled.
 */

int
debug_is_enabled(const char *name);

/**
 * Initialize the given `debugger` with `name`.
 */

int
debug_init(debug_t *debugger, const char *name);

#endif // DEBUG_H
