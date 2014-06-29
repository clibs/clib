
//
// parse-repo.c
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "strdup/strdup.h"
#include "substr/substr.h"
#include "parse-repo.h"

char *
parse_repo_owner(const char *slug, const char *fallback) {
  char *copy = NULL;
  char *owner = NULL;

  if (NULL == slug) return NULL;
  if (0 == strlen(slug)) return NULL;
  if (!(copy = strdup(slug))) return NULL;

  if ((owner = strchr(copy, '/'))) {
    int delta = owner - copy;
    if (!delta) {
      free(copy);
      return NULL;
    }

    char *tmp = substr(copy, 0, delta);
    free(copy);
    return tmp;
  }

  if (fallback && 0 < strlen(copy) && '@' != copy[0]) {
    owner = strdup(fallback);
  }

  free(copy);
  return owner;
}

char *
parse_repo_name(const char *slug) {
  char *owner = NULL;
  char *copy = NULL;
  char *name = NULL;
  char *version = NULL;

  if (NULL == slug) return NULL;
  if (0 == strlen(slug)) return NULL;
  if (!(copy = strdup(slug))) return NULL;

  if ((version = strchr(copy, '@'))) {
    int delta = version - copy;
    char *tmp = substr(copy, 0, delta);
    free(copy);
    if (!tmp) return NULL;
    copy = tmp;
  }

  if ((owner = strchr(copy, '/'))) {
    int delta = owner - copy;
    if (!delta) {
      free(copy);
      return NULL;
    }

    char *tmp = substr(copy, delta + 1, -1);
    free(copy);
    copy = tmp;
  }

  if (copy) name = strdup(copy);
  free(copy);
  return name;
}

char *
parse_repo_version(const char *slug, const char *fallback) {
  // malformed slugs
  if (NULL == slug) return NULL;
  if (0 == strlen(slug)) return NULL;
  char *name = parse_repo_name(slug);
  if (NULL == name) return NULL;
  free(name);

  char *version = strchr(slug, '@');
  if (version) {
    version++;
    // malformed
    if (0 == strlen(version)) return NULL;
    // * <-> master
    if ('*' == version[0]) return strdup("master");
    return strdup(version);
  }

  version = NULL;
  if (fallback) version = strdup(fallback);
  return version;
}
