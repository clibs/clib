
//
// strdup.c
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#ifndef HAVE_STRDUP

#include <stdlib.h>
#include <string.h>
#include "strdup.h"

char *
strdup(const char *str) {
  if (!str) return NULL;
  int len = strlen(str) + 1;
  char *buf = malloc(len);
  if (buf) memcpy(buf, str, len);
  return buf;
}

#endif /* HAVE_STRDUP */
