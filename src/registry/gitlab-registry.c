//
// gitlab-registry.c
//
// Copyright (c) 2021 Elbert van de Put
// MIT licensed
//
#include "gitlab-registry.h"
#include "gumbo-get-element-by-id/get-element-by-id.h"
#include "http-get/http-get.h"
#include "registry-internal.h"
#include <curl/curl.h>
#include <string.h>

/**
 * Parse a list of packages from the given `html`
 */
static list_t *gitlab_registry_parse(const char *hostname, const char *html) {
  list_t *pkgs = list_new();

  // Try to parse the markdown file.
  char *input = strdup(html);
  char *line;
  char *category = NULL;
  while ((line = strsep(&input, "\n"))) {
    char *dash_position = strstr(line, "-");
    // The line starts with a dash, so we expect a package.
    if (dash_position != NULL && dash_position - line < 4) {

      char *link_name_start = strstr(line, "[") + 1;
      char *link_name_end = strstr(link_name_start, "]");
      char *link_value_start = strstr(link_name_end, "(") + 1;
      char *link_value_end = strstr(link_value_start, ")");
      char *description_position = strstr(link_value_end, "-") + 1;

      registry_package_ptr_t package = registry_package_new();
      package->href = strndup(link_value_start, link_value_end - link_value_start);
      package->id = strndup(link_name_start, link_name_end - link_name_start);
      package->description = strdup(description_position);
      package->category = strdup(category != NULL ? category : "unknown");
      list_rpush(pkgs, list_node_new(package));
    }

    char *header_position = strstr(line, "##");
    // The category starts with a ##.
    if (header_position != NULL && header_position - line < 4) {
      category = header_position + 2;
    }
  }

  free(input);

  return pkgs;
}

/**
 * Get a list of packages from the given gitlab file `url`.
 */
list_t *gitlab_registry_fetch(const char *url, const char *hostname, const char *secret) {
  http_get_response_t *res;
  if (secret == NULL) {
    return NULL;
  }

  char *key = "PRIVATE-TOKEN";
  unsigned int size = strlen(key) + strlen(secret) + 2;
  char *authentication_header = malloc(size);
  snprintf(authentication_header, size, "%s:%s", key, secret);
  res = http_get(url, &authentication_header, 1);
  if (!res->ok) {
    return NULL;
  }

  list_t *list = gitlab_registry_parse(hostname, res->data);
  http_get_free(res);
  return list;
}
