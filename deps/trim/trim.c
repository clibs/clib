
//
// trim.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//


#include <ctype.h>
#include <string.h>
#include "trim.h"

char *trim_left(char *str) {
  int len = strlen(str);
  char *cur = str;

  while (*cur && isspace(*cur)) {
    ++cur;
    --len;
  }

  if (str != cur) {
    memmove(str, cur, len + 1);
  }

  return str;
}

char *trim_right(char *str) {
  int len = strlen(str);
  char *cur = str + len - 1;

  while (cur != str && isspace(*cur)) {
    --cur;
    --len;
  }

  cur[isspace(*cur) ? 0 : 1] = '\0';

  return str;
}

char *trim(char *str) {
  trim_right(str);
  trim_left(str);
  return str;
}
