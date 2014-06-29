
//
// path-normalize.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <string.h>
#include "strdup/strdup.h"
#include "path-normalize.h"

/*
 * Normalize the given `path`
 */

char *
path_normalize(const char *path) {
  if (!path) return NULL;

  char *copy = strdup(path);
  if (NULL == copy) return NULL;
  char *ptr = copy;

  for (int i = 0; copy[i]; i++) {
    *ptr++ = path[i];
    if ('/' == path[i]) {
      i++;
      while ('/' == path[i]) i++;
      i--;
    }
  }

  *ptr = '\0';

  return copy;
}
