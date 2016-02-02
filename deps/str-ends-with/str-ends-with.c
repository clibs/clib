
//
// str-ends-with.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <string.h>
#include <stdbool.h>
#include "str-ends-with.h"

bool str_ends_with(const char *str, const char *end) {
  int end_len;
  int str_len;

  if (NULL == str || NULL == end) return false;

  end_len = strlen(end);
  str_len = strlen(str);

  return str_len < end_len
       ? false
       : !strcmp(str + str_len - end_len, end);
}
