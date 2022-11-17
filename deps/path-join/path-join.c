
//
// path-join.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <string.h>
#include <stdlib.h>
#include "strdup/strdup.h"
#include "str-ends-with/str-ends-with.h"
#include "str-starts-with/str-starts-with.h"
#include "path-join.h"

#include <stdio.h>
#include <pthread.h>

#ifdef _WIN32
#define PATH_JOIN_SEPERATOR   "\\"
#else
#define PATH_JOIN_SEPERATOR   "/"
#endif

/*
 * Join `dir` with `file`
 */

char *
path_join(const char *dir, const char *file) {
  int size = strlen(dir) + strlen(file) + 2;
  char *buf = malloc(size * sizeof(char));

  printf("1: size: %u dir: %s, dir-strlen: %lu file: %s file-strlen: %lu file-addr: %p buf-addr %p thread: %lu\n", size, dir, strlen(dir), file, strlen(file), file, buf, pthread_self());

  if (NULL == buf) return NULL;

  strcpy(buf, dir);

  // add the sep if necessary
  if (!str_ends_with(dir, PATH_JOIN_SEPERATOR)) {
    strcat(buf, PATH_JOIN_SEPERATOR);
  }

  // remove the sep if necessary
  if (str_starts_with(file, PATH_JOIN_SEPERATOR)) {
    char *filecopy = strdup(file);
    if (NULL == filecopy) {
      free(buf);
      return NULL;
    }
    strcat(buf, ++filecopy);
    free(--filecopy);
  } else {
    printf("2: buff-size: %lu buff: %s file: %s file-strlen: %lu file-addr: %p buf-adr: %p thread: %lu\n", strlen(buf), buf, file, strlen(file), file, buf, pthread_self());
    strcat(buf, file);
  }

  return buf;
}
