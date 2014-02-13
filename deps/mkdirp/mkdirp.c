
//
// mkdirp.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "str-copy/str-copy.h"
#include "path-normalize/path-normalize.h"
#include "mkdirp.h"

/*
 * Recursively `mkdir(path, mode)`
 */

int mkdirp(const char *path, mode_t mode) {
  if (NULL == path) return -1;

  char *pathname = path_normalize(path);
  if (NULL == pathname) return -1;

  char *parent = str_copy(pathname);
  if (NULL == parent) {
    free(pathname);
    return -1;
  }

  char *p = parent + strlen(parent);
  while ('/' != *p && p != parent) {
    p--;
  }
  *p = '\0';

  // make parent dir
  if (p != parent && 0 != mkdirp(parent, mode)) {
    return -1;
  }

  // make this one if parent has been made
#ifdef _WIN32
  if (0 == mkdir(pathname) || EEXIST == errno) {
    return 0;
  }
#else
  if (0 == mkdir(pathname, mode) || EEXIST == errno) {
    return 0;
  }
#endif

  return -1;
}
