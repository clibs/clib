//
// gitlab-repository.c
//
// Copyright (c) 2021 Elbert van de Put
// MIT licensed
//
#include "gitlab-repository.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <url/url.h>
#include "str-replace/str-replace.h"

#define GITLAB_API_V4_URL "https://%s/api/v4%s/repository/files/%s/raw?ref=%s"

// GET :hostname/api/v4/projects/:id/repository/files/:file_path/raw
char *gitlab_repository_get_url_for_file(const char *package_url, const char *slug, const char *version, const char *file, const char *secret) {
  url_data_t *parsed = url_parse(package_url);

  char* encoded_filename = str_replace(file, "/", "%2F");

  size_t size = strlen(parsed->hostname) + strlen(parsed->pathname) + strlen(encoded_filename) + strlen(GITLAB_API_V4_URL) + strlen(version) + 1;
  char *url = malloc(size);
  if (url) {
    snprintf(url, size, GITLAB_API_V4_URL, parsed->hostname, parsed->pathname, encoded_filename, version);
  }

  url_free(parsed);

  return url;
}
