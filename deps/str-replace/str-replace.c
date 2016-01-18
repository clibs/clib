
//
// str-replace.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <stdlib.h>
#include <string.h>
#include "occurrences/occurrences.h"
#include "strdup/strdup.h"
#include "str-replace.h"

/*
 * Replace all occurrences of `sub` with `replace` in `str`
 */

char *
str_replace(const char *str, const char *sub, const char *replace) {
  char *pos = (char *) str;
  int count = occurrences(sub, str);

  if (0 >= count) return strdup(str);

  int size = (
        strlen(str)
      - (strlen(sub) * count)
      + strlen(replace) * count
    ) + 1;

  char *result = (char *) malloc(size);
  if (NULL == result) return NULL;
  memset(result, '\0', size);
  char *current;
  while ((current = strstr(pos, sub))) {
    int len = current - pos;
    strncat(result, pos, len);
    strncat(result, replace, strlen(replace));
    pos = current + strlen(sub);
  }

  if (pos != (str + strlen(str))) {
    strncat(result, pos, (str - pos));
  }

  return result;
}
