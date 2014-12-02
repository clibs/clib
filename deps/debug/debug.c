
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "strdup/strdup.h"
#include "asprintf/asprintf.h"
#include "wildcardcmp/wildcardcmp.h"
#include "debug.h"

/**
 * Have we seeded `rand()`?
 */

static int seeded = 0;

/**
 * Log format.
 */

#define FMT "\x1B[3%dm%s\033[0m"

/**
 * ENV var delimiter.
 */

#define DELIMITER ","

/**
 * Check if the given `stream` is a TTY, and/or if
 * the environment variable `DEBUG_COLORS` has been
 * disabled.
 */

static int
use_colors(FILE *stream) {
  // only use color for a TTY
  int tty = isatty(fileno(stream));
  if (0 == tty) return 0;
  // check env
  char *colors = getenv("DEBUG_COLORS");
  if (!colors) return 1;
  // see: https://github.com/visionmedia/debug/blob/2.1.0/node.js#L49-L52
  if (0 == strcmp("0", colors)) return 0;
  if (0 == strncmp("no", colors, 2)) return 0;
  if (0 == strncmp("false", colors, 5)) return 0;
  if (0 == strncmp("disabled", colors, 8)) return 0;
  return 1;
}

int
debug_is_enabled(const char *name) {
  char *env = NULL;
  char *tmp = NULL;
  char *debugger = NULL;
  int enabled = 0;

  // get DEBUG env var
  if (!(env = getenv("DEBUG"))) return 0;
  if (!(tmp = strdup(env))) return 0;

  debugger = strtok(tmp, DELIMITER);
  while (debugger) {
    // support DEBUG=foo*
    if (1 == wildcardcmp(debugger, name)) {
      enabled = 1;
      break;
    }
    debugger = strtok(NULL, DELIMITER);
  }

  free(tmp);
  return enabled;
}

int
debug_init(debug_t *debugger, const char *name) {
  // seed, if necessary
  if (0 == seeded) {
    srand(clock());
    seeded = 1;
  }

  // random color
  debugger->color = 1 + (rand() % 6);
  // set enabled flag
  debugger->enabled = debug_is_enabled(name);
  // name, stream
  debugger->name = name;
  debugger->stream = stderr;
  return 0;
}

void
debug(debug_t *debugger, const char *fmt, ...) {
  // noop when disabled
  if (0 == debugger->enabled) return;

  char *pre = NULL;
  char *post = NULL;
  va_list args;

  va_start(args, fmt);

  if (use_colors(debugger->stream)) {
    // [color][name][/color]
    if (-1 == asprintf(&pre, FMT, debugger->color, debugger->name)) {
      va_end(args);
      return;
    }
  } else {
    // [name]
    if (!(pre = strdup(debugger->name))) {
      va_end(args);
      return;
    }
  }

  // format args
  if (-1 == vasprintf(&post, fmt, args)) {
    free(pre);
    va_end(args);
    return;
  }

  // print to stream
  fprintf(debugger->stream, " %s : %s\n", pre, post);

  // release memory
  free(pre);
  free(post);
}
