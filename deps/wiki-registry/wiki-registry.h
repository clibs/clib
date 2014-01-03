
//
// wiki-registry.h
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//


#ifndef WIKI_REGISTRY_H
#define WIKI_REGISTRY_H 1

#include "list/list.h"

typedef struct {
  char *repo;
  char *href;
  char *description;
  char *category;
} wiki_package_t;

list_t *
wiki_registry(const char *);

list_t *
wiki_registry_parse(const char *);

void
wiki_package_free(wiki_package_t *);

#endif
