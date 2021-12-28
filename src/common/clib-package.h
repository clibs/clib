//
// clib-package.h
//
// Copyright (c) 2014 Stephen Mathieson
// Copyright (c) 2014-2020 clib authors
// MIT license
//

#ifndef CLIB_PACKAGE_H
#define CLIB_PACKAGE_H 1

#include "list/list.h"
#include <curl/curl.h>

typedef struct {
  char *name;
  char *author;
  char *version;
} clib_package_dependency_t;

typedef struct {
  char *author;
  char *description;
  char *install;
  char *configure;
  char *json;
  char *license;
  char *name;
  char *repo;
  char *repo_name;
  char *url;
  char *version;
  char *makefile;
  char *filename; // `package.json` or `clib.json`
  char *flags;
  char *prefix;
  list_t *dependencies;
  list_t *development;
  list_t *src;
  list_t *registries;
  void *data; // user data
  unsigned int refs;
} clib_package_t;

typedef struct {
  int skip_cache;
  int force;
  int global;
  char *prefix;
  int concurrency;
  char *token;
} clib_package_opts_t;

extern CURLSH *clib_package_curl_share;
// TODO, move to a separate file.
extern clib_package_opts_t package_opts;

void clib_package_set_opts(clib_package_opts_t opts);

clib_package_t *clib_package_new(const char *, int);

clib_package_t *clib_package_new_from_slug_and_url(const char* slug, const char* url, int);

clib_package_t *clib_package_load_from_manifest(const char *, int);

clib_package_t *clib_package_load_local_manifest(int);

char *clib_package_parse_version(const char *);

char *clib_package_parse_author(const char *);

char *clib_package_parse_name(const char *);

clib_package_dependency_t *clib_package_dependency_new(const char *,
                                                       const char *);

char *clib_package_get_id(const char *, const char *);

char *clib_package_slug(const char *author, const char *name, const char* version);

void clib_package_set_prefix(clib_package_t *pkg, long path_max);

void clib_package_free(clib_package_t *);

void clib_package_dependency_free(void *);

void clib_package_cleanup();

#endif
