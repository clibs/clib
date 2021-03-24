//
// registry-manager.c
//
// Copyright (c) 2021 Elbert van de Put
// MIT licensed
//
#include "registry-manager.h"
#include <stdlib.h>
#include "url/url.h"

#define CLIB_WIKI_URL "https://github.com/clibs/clib/wiki/Packages"

registries_t registry_manager_init_registries(list_t* registry_urls, clib_secrets_t secrets) {
  list_t* registries = list_new();

  // Add all the registries that were provided.
  list_iterator_t *registry_iterator = list_iterator_new(registry_urls, LIST_HEAD);
  list_node_t *node;
  while ((node = list_iterator_next(registry_iterator))) {
    char* url = node->val;
    url_data_t *parsed = url_parse(url);
    char* hostname = strdup(parsed->hostname);
    url_free(parsed);
    char* secret = clib_secret_find_for_hostname(secrets, hostname);
    registry_ptr_t registry = registry_create(url, secret);
    list_rpush(registries, list_node_new(registry));
  }
  list_iterator_destroy(registry_iterator);

  // And add the default registry.
  registry_ptr_t registry = registry_create(CLIB_WIKI_URL, NULL);
  list_rpush(registries, list_node_new(registry));

  return registries;
}

void registry_manager_fetch_registries(registries_t registries) {
  registry_iterator_t it = registry_iterator_new(registries);
  registry_ptr_t reg;
  while ((reg = registry_iterator_next(it))) {
      if (!registry_fetch(reg)) {
      printf("REGISTRY: could not list packages from. %s\n", registry_get_url(reg));
    }
  }
  registry_iterator_destroy(it);
}

registry_package_ptr_t registry_manger_find_package(registries_t registries, const char* package_id) {
  registry_iterator_t it = registry_iterator_new(registries);
  registry_ptr_t reg;
  while ((reg = registry_iterator_next(it))) {
    registry_package_ptr_t package = registry_find_package(reg, package_id);
    if (package != NULL) {
      registry_iterator_destroy(it);
      return package;
    }
  }
  registry_iterator_destroy(it);

  return NULL;
}

registry_iterator_t registry_iterator_new(registries_t registries) {
    return list_iterator_new(registries, LIST_HEAD);
}

registry_ptr_t registry_iterator_next(registry_iterator_t iterator) {
  list_node_t *node = list_iterator_next(iterator);
  if (node == NULL) {
    return NULL;
  }

  return (registry_ptr_t) node->val;
}

void registry_iterator_destroy(registry_iterator_t iterator) {
  list_iterator_destroy(iterator);
}
