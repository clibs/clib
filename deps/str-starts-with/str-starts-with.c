
//
// str-starts-with.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <stdbool.h>
#include <string.h>
#include "str-starts-with.h"

bool str_starts_with(const char *str, const char *start) {
  int str_len = strlen(str);
  int start_len = strlen(start);

  return str_len < start_len
       ? false
       : 0 == strncmp(start, str, start_len);
}

