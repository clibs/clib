
//
// clib-search.c
//
// Copyright (c) 2012-2014 clib authors
// MIT licensed
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "case/case.h"
#include "commander/commander.h"
#include "tempdir/tempdir.h"
#include "fs/fs.h"
#include "http-get/http-get.h"
#include "asprintf/asprintf.h"
#include "wiki-registry/wiki-registry.h"
#include "clib-package/clib-package.h"
#include "console-colors/console-colors.h"
#include "strdup/strdup.h"
#include "logger/logger.h"
#include "debug/debug.h"
#include "version.h"

#define CLIB_WIKI_URL "https://github.com/clibs/clib/wiki/Packages"
#define CLIB_SEARCH_CACHE "clib-search.cache"
#define CLIB_SEARCH_CACHE_TIME 1000 * 60 * 60 * 5

debug_t debugger;

static int opt_color;
static int opt_cache;

static void
setopt_nocolor(command_t *self) {
    opt_color = 0;
}

static void
setopt_nocache(command_t *self) {
    opt_cache = 0;
}

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
    if (strstr(name, args[i])) {
      free(name);
      return 1;
    }
  }

  description = strdup(pkg->description);
  if (NULL == description) goto fail;
  case_lower(description);
  for (int i = 0; i < count; i++) {
    if (strstr(description, args[i])) {
      free(description);
      free(name);
      return 1;
    }
  }

fail:
  free(name);
  free(description);
  return 0;
}

static char *
clib_search_file(void) {
  char *file = NULL;
  char *temp = NULL;

  temp = gettempdir();
  if (NULL == temp) {
    logger_error("error", "gettempdir() out of memory");
    return NULL;
  }

  debug(&debugger, "tempdir: %s", temp);
  int rc = asprintf(&file, "%s/%s", temp, CLIB_SEARCH_CACHE);
  if (-1 == rc) {
    logger_error("error", "asprintf() out of memory");
    free(temp);
    return NULL;
  }

  free(temp);
  debug(&debugger, "search file: %s", file);
  return file;
}

static char *
wiki_html_cache() {
  char *cache_file = clib_search_file();
  if (NULL == cache_file) return NULL;

  if (0 == opt_cache) {
    debug(&debugger, "skipping cache file (%s)", cache_file);
    goto set_cache;
  }

  fs_stats *stats = fs_stat(cache_file);
  if (NULL == stats) goto set_cache;

  long now = (long) time(NULL);
  long modified = stats->st_mtime;
  long delta = now - modified;

  debug(&debugger, "cache delta %d (%d - %d)", delta, now, modified);
  free(stats);

  if (delta < CLIB_SEARCH_CACHE_TIME) {
    char *data = fs_read(cache_file);
    free(cache_file);
    return data;
  }

set_cache:;
  debug(&debugger, "setting cache (%s) from %s", cache_file, CLIB_WIKI_URL);
  http_get_response_t *res = http_get(CLIB_WIKI_URL);
  if (!res->ok) return NULL;

  char *html = strdup(res->data);
  if (NULL == html) return NULL;
  http_get_free(res);

  if (NULL == html) return html;
  fs_write(cache_file, html);
  debug(&debugger, "wrote cache (%s)", cache_file);
  free(cache_file);
  return html;
}

int
main(int argc, char *argv[]) {
  opt_color = 1;
  opt_cache = 1;

  debug_init(&debugger, "clib-search");

  command_t program;
  command_init(&program, "clib-search", CLIB_VERSION);
  program.usage = "[options] [query ...]";

  command_option(
      &program
    , "-n"
    , "--no-color"
    , "don't colorize output"
    , setopt_nocolor
  );

  command_option(
      &program
    , "-c"
    , "--skip-cache"
    , "skip the search cache"
    , setopt_nocache
  );

  command_parse(&program, argc, argv);

  for (int i = 0; i < program.argc; i++) case_lower(program.argv[i]);

  // set color theme
  cc_color_t fg_color_highlight = opt_color
    ? CC_FG_DARK_CYAN
    : CC_FG_NONE;
  cc_color_t fg_color_text = opt_color
    ? CC_FG_DARK_GRAY
    : CC_FG_NONE;

  char *html = wiki_html_cache();
  if (NULL == html) {
    command_free(&program);
    logger_error("error", "failed to fetch wiki HTML");
    return 1;
  }

  list_t *pkgs = wiki_registry_parse(html);
  free(html);

  debug(&debugger, "found %zu packages", pkgs->len);

  list_node_t *node;
  list_iterator_t *it = list_iterator_new(pkgs, LIST_HEAD);
  printf("\n");
  while ((node = list_iterator_next(it))) {
    wiki_package_t *pkg = (wiki_package_t *) node->val;
    if (matches(program.argc, program.argv, pkg)) {
      cc_fprintf(fg_color_highlight, stdout, "  %s\n", pkg->repo);
      printf("  url: ");
      cc_fprintf(fg_color_text, stdout, "%s\n", pkg->href);
      printf("  desc: ");
      cc_fprintf(fg_color_text, stdout, "%s\n", pkg->description);
      printf("\n");
    } else {
      debug(&debugger, "skipped package %s", pkg->repo);
    }
    wiki_package_free(pkg);
  }
  list_iterator_destroy(it);
  list_destroy(pkgs);
  command_free(&program);
  return 0;
}
