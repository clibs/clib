
//
// clib-package.h
//
// Copyright (c) 2014 Stephen Mathieson
// MIT license
//

#ifndef CLIB_PACKAGE_H
#define CLIB_PACKAGE_H 1

#include "list/list.h"

typedef struct {
  char *name;
  char *author;
  char *version;
} clib_package_dependency_t;

typedef struct {
  char *author;
  char *description;
  char *install;
  char *json;
  char *license;
  char *name;
  char *repo;
  char *repo_name;
  char *url;
  char *version;
  char *makefile;
  list_t *dependencies;
  list_t *development;
  list_t *src;
} clib_package_t;

clib_package_t *
clib_package_new(const char *, int);

clib_package_t *
clib_package_new_from_slug(const char *, int);

char *
clib_package_url(const char *, const char *, const char *);

char *
clib_package_url_from_repo(const char *repo, const char *version);

char *
clib_package_parse_version(const char *);

char *
clib_package_parse_author(const char *);

char *
clib_package_parse_name(const char *);

clib_package_dependency_t *
clib_package_dependency_new(const char *, const char *);

int
clib_package_install(clib_package_t *, const char *, int);

int
clib_package_install_dependencies(clib_package_t *, const char *, int);

int
clib_package_install_development(clib_package_t *, const char *, int);

void
clib_package_free(clib_package_t *);

void
clib_package_dependency_free(void *);

#endif
