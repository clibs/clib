//
// which.c
//
// Copyright (c) 2013 TJ Holowaychuk <tj@vision-media.ca>
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "which.h"

// delimiter

#ifdef _WIN32
#define PATH_SEPARATOR    "\\"
#define WHICH_DELIMITER   ";"
#else
#define PATH_SEPARATOR    "/"
#define WHICH_DELIMITER   ":"
#endif

/*
 * Lookup executable `name` within the PATH environment variable.
 */

char *
which(const char *name) {
#ifdef _WIN32
  char *bin = which_path(name, ".");
  if (bin) return bin;
#endif
  return which_path(name, getenv("PATH"));
}

/*
 * Lookup executable `name` within `path`.
 */

char *
which_path(const char *name, const char *_path) {
  char *path = strdup(_path);
  char *tok = strtok(path, WHICH_DELIMITER);

  while (tok) {
    // path
    int len = strlen(tok) + 2 + strlen(name);
    char *file = malloc(len);
    if (!file) {
      free(path);
      return NULL;
    }
    sprintf(file, "%s" PATH_SEPARATOR "%s", tok, name);

    // executable
    if (0 == access(file, X_OK)) {
      free(path);
      return file;
    }

    // next token
    tok = strtok(NULL, WHICH_DELIMITER);
    free(file);
  }

  free(path);

  return NULL;
}
