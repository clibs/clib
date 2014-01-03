
//
// case.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "case.h"

#define CASE_MODIFIER     'a' - 'A'
#define CASE_IS_LOWER(c)  c >= 'a' && c <= 'z'
#define CASE_IS_UPPER(c)  c >= 'A' && c <= 'Z'
#define CASE_IS_SEP(c)    c == '-' || c == '_' || c == ' '

char *case_upper(char *str) {
  for (int i = 0, len = strlen(str); i < len; i++) {
    if (CASE_IS_LOWER(str[i])) {
      str[i] -= CASE_MODIFIER;
    }
  }
  return str;
}

char *case_lower(char *str) {
  for (int i = 0, len = strlen(str); i < len; i++) {
    if (CASE_IS_UPPER(str[i])) {
      str[i] += CASE_MODIFIER;
    }
  }
  return str;
}

char *case_camel(char *str) {
  for (int i = 0, len = strlen(str); i < len; i++) {
    if (CASE_IS_SEP(str[i])) {
      memmove(&str[i], &str[i + 1], len - i);
      // never cap the first char
      if (i && CASE_IS_LOWER(str[i])) {
        str[i] -= CASE_MODIFIER;
      }
      // account for removing seperator
      i--;
      len--;
    }
  }

  return str;
}
