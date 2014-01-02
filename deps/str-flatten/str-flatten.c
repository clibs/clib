
//
// str-flatten.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <stdlib.h>
#include <string.h>
#include "str-flatten.h"

char *str_flatten(char *array[], int start, int end) {
  int size = 0;
  for (int i = start; i < end; i++) size += strlen(array[i]);
  size += end - start - 1; // number of spaces

  char *str = malloc(sizeof(char) * size);
  if (NULL == str) return NULL;
  int pos = 0;

  for (int i = start; i < end; i++) {
    char *word = array[i];
    int len = strlen(word);
    strncat(&str[pos], word, len);
    pos += len;
    str[pos] = ' ';
    pos++;
  }

  pos--;
  str[pos] = '\0';

  return str;
}
