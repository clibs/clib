
//
// strdup.h
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#ifndef HAVE_STRDUP
#define HAVE_STRDUP

/**
 * Drop-in replacement for strdup(3) from libc.
 *
 * Creates a copy of `str`.  Free when done.
 *
 * Returns a pointer to the newly allocated
 * copy of `str`, or `NULL` on failure.
 */
#include <stddef.h>
#ifndef strdup
char * strdup(const char *str);
char * strndup(const char *str, size_t len);
#endif

#endif /* HAVE_STRDUP */
