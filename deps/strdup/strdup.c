
//
// strdup.c
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#ifndef HAVE_STRDUP

#include "strdup.h"
#include <stdlib.h>
#include <string.h>

#ifndef strdup

char *
strdup(const char *str) {
  if (NULL == (char *) str) {
    return NULL;
  }

  int len = strlen(str) + 1;
  char *buf = malloc(len);

  if (buf) {
    memset(buf, 0, len);
    memcpy(buf, str, len - 1);
  }
  return buf;
}

char* strndup(const char *str, size_t len) {
  if (NULL == (char *) str) {
    return NULL;
  }

  char *buf = malloc(len+1);

  if (buf) {
    memset(buf, 0, len+1);
    memcpy(buf, str, len);
  }
  return buf;
}
#endif

#endif /* HAVE_STRDUP */
