//
// registry.c
//
// Copyright (c) 2020 clib authors
// MIT licensed
//

#include "github-registry.h"
#include "gitlab-registry.h"
#include "gumbo-parser/gumbo.h"
#include "list/list.h"
#include "registry-internal.h"
#include "url/url.h"
#include <logger/logger.h>
#include <strdup/strdup.h>
#include <stdlib.h>
#include <string.h>

enum registry_type_t {
  REGISTRY_TYPE_GITHUB,
  REGISTRY_TYPE_GITLAB,
};

struct registry_t {
  enum registry_type_t type;
  char *url;
  char *hostname;
  char *secret;
  list_t *packages;
};

/**
 * Create a new registry package.
 */
registry_package_ptr_t registry_package_new() {
  registry_package_ptr_t pkg = malloc(sizeof(struct registry_package_t));
  if (pkg) {
    pkg->id = NULL;
    pkg->href = NULL;
    pkg->description = NULL;
    pkg->category = NULL;
  }
  return pkg;
}

/**
 * Release the memory held by the package.
 */
void registry_package_free(registry_package_ptr_t pkg) {
  free(pkg->id);
  free(pkg->href);
  free(pkg->description);
  free(pkg->category);
  free(pkg);
}

registry_ptr_t registry_create(const char *url, const char *secret) {
  registry_ptr_t registry = malloc(sizeof(struct registry_t));
  registry->url = strdup(url);
  registry->secret = strdup(secret);
  registry->packages = NULL;
  url_data_t *parsed = url_parse(url);
  registry->hostname = strdup(parsed->hostname);
  url_free(parsed);

  if (strstr(registry->hostname, "github.com") != NULL) {
    registry->type = REGISTRY_TYPE_GITHUB;
  } else if (strstr(registry->hostname, "gitlab") != NULL) {
    registry->type = REGISTRY_TYPE_GITLAB;
  } else {
    logger_error("error", "Registry type (%s) not supported, currently github.com, gitlab.com and self-hosted gitlab are supported.", registry->url);
    registry_free(registry);

    return NULL;
  }

  return registry;
}

void registry_free(registry_ptr_t registry) {
  free(registry->url);
  free(registry->hostname);
  if (registry->packages != NULL) {
    list_iterator_t *it = list_iterator_new(registry->packages, LIST_HEAD);
    list_node_t *node;
    while ((node = list_iterator_next(it))) {
      registry_package_free(node->val);
    }
    list_iterator_destroy(it);
    list_destroy(registry->packages);
  }
  free(registry);
}

const char *registry_get_url(registry_ptr_t registry) {
  return registry->url;
}

bool registry_fetch(registry_ptr_t registry) {
  switch (registry->type) {
  case REGISTRY_TYPE_GITLAB:
    registry->packages = gitlab_registry_fetch(registry->url, registry->hostname, registry->secret);
    if (registry->packages != NULL) {
      return true;
    }
    break;
  case REGISTRY_TYPE_GITHUB:
    registry->packages = github_registry_fetch(registry->url);
    if (registry->packages != NULL) {
      return true;
    }
    break;
  default:
    registry->packages = list_new();
    return false;
  }

  logger_error("error", "Fetching package list from (%s) failed.", registry->url);
  registry->packages = list_new();
  return false;
}

registry_package_iterator_t registry_package_iterator_new(registry_ptr_t registry) {
  return list_iterator_new(registry->packages, LIST_HEAD);
}

registry_package_ptr_t registry_package_iterator_next(registry_package_iterator_t iterator) {
  list_node_t *node = list_iterator_next(iterator);
  if (node == NULL) {
    return NULL;
  }

  return (registry_package_ptr_t) node->val;
}

void registry_package_iterator_destroy(registry_package_iterator_t iterator) {
  list_iterator_destroy(iterator);
}

registry_package_ptr_t registry_find_package(registry_ptr_t registry, const char *package_id) {
  registry_package_iterator_t it = registry_package_iterator_new(registry);
  registry_package_ptr_t pack;
  while ((pack = registry_package_iterator_next(it))) {
    if (0 == strcmp(package_id, pack->id)) {
      registry_package_iterator_destroy(it);
      return pack;
    }
  }
  registry_package_iterator_destroy(it);

  return NULL;
}

char *registry_package_get_id(registry_package_ptr_t package) {
  return package->id;
}

char *registry_package_get_href(registry_package_ptr_t package) {
  return package->href;
}

char *registry_package_get_description(registry_package_ptr_t package) {
  return package->description;
}

char *registry_package_get_category(registry_package_ptr_t package) {
  return package->category;
}