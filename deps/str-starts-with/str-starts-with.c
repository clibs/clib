
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
  for (; ; str++, start++)
    if (!*start)
      return true;
    else if (*str != *start)
      return false;
}
