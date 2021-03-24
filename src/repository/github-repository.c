//
// github-repository.c
//
// Copyright (c) 2021 Elbert van de Put
// MIT licensed
//
#include "github-repository.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GITHUB_CONTENT_URL "https://raw.githubusercontent.com/"
#define GITHUB_CONTENT_URL_WITH_TOKEN "https://%s@raw.githubusercontent.com/"

char* github_repository_get_url_for_file(const char* hostname, const char* slug, const char* version, const char *file_path, const char* secret) {

  int size = strlen(GITHUB_CONTENT_URL) + strlen(slug) + 1 // /
      + strlen(version) + 1                         // \0
  ;

  if (secret != NULL) {
    size += strlen(secret);
    size += 1; // @
  }

  char *url = malloc(size);
  if (url) {
    memset(url, '\0', size);
    if (secret != NULL) {
      sprintf(url, GITHUB_CONTENT_URL_WITH_TOKEN "%s/%s", secret, slug, version);
    } else {
      sprintf(url, GITHUB_CONTENT_URL "%s/%s", slug, version);
    }
  }

  return url;
}
