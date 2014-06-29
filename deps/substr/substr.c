
//
// substr.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <stdlib.h>
#include <string.h>
#include "strdup/strdup.h"
#include "substr.h"

/*
 * Get a substring of `str` from `start` to `end`
 */

char *
substr(const char *str, int start, int end) {
  if (0 > start) return NULL;
  int len = strlen(str);
  // -1 == length of string
  if (-1 == end) end = len;
  if (end <= start) return NULL;
  int diff = end - start;
  if (len == diff) return strdup(str);
  if (len < start) return NULL;
  if (len + 1 < end) return NULL;

  char *res = malloc(sizeof(char) * diff + 1);
  if (NULL == res) return NULL;
  memset(res, '\0', diff + 1);
  strncpy(res, str + start, diff);
  return res;
}
