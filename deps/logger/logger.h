
//
// logger.h
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#ifndef CLIB_LOGGER_H
#define CLIB_LOGGER_H 1

#include <stdio.h>
#include "console-colors/console-colors.h"

#ifndef CLIB_LOGGER_FMT
#  define CLIB_LOGGER_FMT "  %10s"
#endif

/**
 * Log an info message to stdout.
 */

#define logger_info(type, ...) ({                          \
  cc_fprintf(CC_FG_CYAN, stdout, CLIB_LOGGER_FMT, type);   \
  fprintf(stdout, " : ");                                  \
  cc_fprintf(CC_FG_DARK_GRAY, stdout, __VA_ARGS__);        \
  fprintf(stdout, "\n");                                   \
});

/**
 * Log a warning to stdout.
 */

#define logger_warn(type, ...) ({                               \
  cc_fprintf(CC_FG_DARK_YELLOW, stdout, CLIB_LOGGER_FMT, type); \
  fprintf(stdout, " : ");                                       \
  cc_fprintf(CC_FG_DARK_GRAY, stdout, __VA_ARGS__);             \
  fprintf(stdout, "\n");                                        \
});

/**
 * Log an error message to stderr.
 */

#define logger_error(type, ...) ({                           \
  cc_fprintf(CC_FG_DARK_RED, stderr, CLIB_LOGGER_FMT, type); \
  fprintf(stderr, " : ");                                    \
  cc_fprintf(CC_FG_DARK_GRAY, stderr, __VA_ARGS__);          \
  fprintf(stderr, "\n");                                     \
});

#endif
