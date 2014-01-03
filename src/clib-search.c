
//
// clib-search.c
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "case/case.h"
#include "commander/commander.h"
#include "fs/fs.h"
#include "http-get/http-get.h"
#include "wiki-registry/wiki-registry.h"
#include "clib-package/clib-package.h"
#include "version.h"

#define CLIB_WIKI_URL "https://github.com/clibs/clib/wiki/Packages"
#define CLIB_SEARCH_CACHE "/tmp/clib-search.cache"
#define CLIB_SEARCH_CACHE_TIME 1000 * 60 * 60 * 5

static int
matches(int count, char *args[], wiki_package_t *pkg) {
  // Display all packages if there's no query
  if (0 == count) return 1;

  char *name = NULL;
  char *description = NULL;

  name = clib_package_parse_name(pkg->repo);
  if (NULL == name) goto fail;
  case_lower(name);
  for (int i = 0; i < count; i++) {
    if (strstr(name, args[i])) return 1;
  }

  description = strdup(pkg->description);
  if (NULL == description) goto fail;
  case_lower(description);
  for (int i = 0; i < count; i++) {
    if (strstr(description, args[i])) {
      free(description);
      return 1;
    }
  }

fail:
  if (description) free(description);
  return 0;
}

static char *
wiki_html_cache() {
  fs_stats *stats = fs_stat(CLIB_SEARCH_CACHE);
  if (NULL == stats) goto set_cache;

  long now = (long) time(NULL);
  long modified = stats->st_mtime;
  long delta = now - modified;

  free(stats);

  if (delta < CLIB_SEARCH_CACHE_TIME) return fs_read(CLIB_SEARCH_CACHE);

set_cache:;
  http_get_response_t *res = http_get(CLIB_WIKI_URL);
  if (!res->ok) return NULL;

  char *html = strdup(res->data);
  http_get_free(res);

  if (NULL == html) return html;
  fs_write(CLIB_SEARCH_CACHE, html);
  return html;
}

int
main(int argc, char *argv[]) {
  command_t program;
  command_init(&program, "clib-search", CLIB_VERSION);
  program.usage = "[options] [query ...]";
  command_parse(&program, argc, argv);

  for (int i = 0; i < program.argc; i++) case_lower(program.argv[i]);

  char *html = wiki_html_cache();
  if (NULL == html) {
    command_free(&program);
    fprintf(stderr, "Failed to fetch wiki HTML\n");
    return 1;
  }

  list_t *pkgs = wiki_registry_parse(html);
  free(html);

  list_node_t *node;
  list_iterator_t *it = list_iterator_new(pkgs, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    wiki_package_t *pkg = (wiki_package_t *) node->val;
    if (matches(program.argc, program.argv, pkg)) {
      printf("  \033[36m%s\033[m\n", pkg->repo);
      printf("  url: \033[90m%s\033[m\n", pkg->href);
      printf("  desc: \033[90m%s\033[m\n", pkg->description);
      printf("\n");
    }
    wiki_package_free(pkg);
  }
  list_iterator_destroy(it);
  list_destroy(pkgs);
  command_free(&program);
  return 0;
}
