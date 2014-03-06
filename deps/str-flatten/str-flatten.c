
//
// str-flatten.c
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#include <stdlib.h>
#include <string.h>
#include "str-flatten.h"

char *
str_flatten(const char *array[], int start, int end) {
  int count = end - start;
  size_t lengths[count];
  size_t size = 0;
  size_t pos = 0;

  for (int i = start, j = 0; i < end; ++i, ++j) {
    lengths[j] = strlen(array[i]);
    size += lengths[j];
  }

  char *str = malloc(size + count);
  str[size + count - 1] = '\0';

  for (int i = start, j = 0; i < (end - 1); ++i, ++j) {
    memcpy(str + pos + j
      // current index
      , array[i]
      // current index length
      , lengths[j]);
    // add space
    str[pos + lengths[j] + j] = ' ';
    // bump `pos`
    pos += lengths[j];
  }

  memcpy(str + pos + count - 1
    , array[end - 1]
    , lengths[count - 1]);

  return str;
}
