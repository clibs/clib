//
// clib-install.c
//
// Copyright (c) 2012-2020 clib authors
// MIT licensed
//

#include "commander/commander.h"
#include "common/clib-cache.h"
#include "common/clib-package.h"
#include "debug/debug.h"
#include "fs/fs.h"
#include "http-get/http-get.h"
#include "logger/logger.h"
#include "parson/parson.h"
#include "str-concat/str-concat.h"
#include "str-replace/str-replace.h"
#include "tempdir/tempdir.h"
#include "version.h"
#include <asprintf/asprintf.h>
#include <curl/curl.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLIB_PACKAGE_CACHE_TIME 30 * 24 * 60 * 60

#define SX(s) #s
#define S(s) SX(s)

#ifdef HAVE_PTHREADS
#define MAX_THREADS 16
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) ||               \
    defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(k, v, _) _putenv_s(k, v)
#define realpath(a, b) _fullpath(a, b, strlen(a))
#endif

extern CURLSH *clib_package_curl_share;

debug_t debugger = {0};

struct options {
  char *prefix;
  char *token;
  char *slug;
  char *tag;
  char *dir;
  int verbose;
  int force;
#ifdef HAVE_PTHREADS
  unsigned int concurrency;
#endif
};

static struct options opts = {0};

static const char *manifest_names[] = {"clib.json", "package.json", NULL};

static clib_package_opts_t package_opts = {0};
static clib_package_t *root_package = NULL;

/**
 * Option setters.
 */

static void setopt_slug(command_t *self) {
  opts.slug = (char *)self->arg;
  debug(&debugger, "set slug: %s", opts.slug);
}

static void setopt_tag(command_t *self) {
  opts.tag = (char *)self->arg;
  debug(&debugger, "set tag: %s", opts.tag);
}

static void setopt_prefix(command_t *self) {
  opts.prefix = (char *)self->arg;
  debug(&debugger, "set prefix: %s", opts.prefix);
}

static void setopt_token(command_t *self) {
  opts.token = (char *)self->arg;
  debug(&debugger, "set token: %s", opts.token);
}

static void setopt_quiet(command_t *self) {
  opts.verbose = 0;
  debug(&debugger, "set quiet flag");
}

static void setopt_force(command_t *self) {
  opts.force = 1;
  debug(&debugger, "set force flag");
}

#ifdef HAVE_PTHREADS
static void setopt_concurrency(command_t *self) {
  if (self->arg) {
    opts.concurrency = atol(self->arg);
    debug(&debugger, "set concurrency: %lu", opts.concurrency);
  }
}
#endif

/**
 * Create and install a package from `slug`.
 */

static int install_package(const char *slug) {
  clib_package_t *pkg = NULL;
  int rc;

  if (!root_package) {
    const char *name = NULL;
    char *json = NULL;
    unsigned int i = 0;

    do {
      name = manifest_names[i];
      json = fs_read(name);
    } while (NULL != manifest_names[++i] && !json);

    if (json) {
      root_package = clib_package_new(json, opts.verbose);
    }
  }

  char *extended_slug = 0;
  if (0 != opts.tag) {
    asprintf(&extended_slug, "%s@%s", slug, opts.tag);
  }

  if (0 != extended_slug) {
    pkg = clib_package_new_from_slug(extended_slug, opts.verbose);
  } else {
    pkg = clib_package_new_from_slug(slug, opts.verbose);
  }

  if (NULL == pkg)
    return -1;

  if (root_package && root_package->prefix) {
    package_opts.prefix = root_package->prefix;
    clib_package_set_opts(package_opts);
  }

  char *tmp = gettempdir();

  if (0 != tmp) {
    rc = clib_package_install(pkg, tmp, opts.verbose);
  } else {
    rc = -1;
    goto cleanup;
  }

  if (0 != rc) {
    goto cleanup;
  }

  if (0 == pkg->repo || 0 != strcmp(slug, pkg->repo)) {
    pkg->repo = strdup(slug);
  }

cleanup:
  if (0 != extended_slug) {
    free(extended_slug);
  }
  clib_package_free(pkg);
  return rc;
}

/**
 * Entry point.
 */

int main(int argc, char *argv[]) {
  opts.verbose = 1;

  long path_max = 4096;

  debug_init(&debugger, "clib-upgrade");

  // 30 days expiration
  clib_cache_init(CLIB_PACKAGE_CACHE_TIME);

  command_t program;

  command_init(&program, "clib-upgrade", CLIB_VERSION);

  program.usage = "[options] [name ...]";

  command_option(&program, "-P", "--prefix <dir>",
                 "change the prefix directory (usually '/usr/local')",
                 setopt_prefix);
  command_option(&program, "-q", "--quiet", "disable verbose output",
                 setopt_quiet);
  command_option(&program, "-f", "--force",
                 "force the action of something, like overwriting a file",
                 setopt_force);
  command_option(&program, "-t", "--token <token>",
                 "Access token used to read private content", setopt_token);
  command_option(&program, "-S", "--slug <slug>",
                 "The slug where the clib project lives (usually 'clibs/clib')",
                 setopt_slug);
  command_option(&program, "-T", "--tag <tag>",
                 "The tag to upgrade to (usually it is the latest)",
                 setopt_token);
#ifdef HAVE_PTHREADS
  command_option(&program, "-C", "--concurrency <number>",
                 "Set concurrency (default: " S(MAX_THREADS) ")",
                 setopt_concurrency);
#endif
  command_parse(&program, argc, argv);

  debug(&debugger, "%d arguments", program.argc);

  if (0 != curl_global_init(CURL_GLOBAL_ALL)) {
    logger_error("error", "Failed to initialize cURL");
  }

  if (opts.prefix) {
    char prefix[path_max];
    memset(prefix, 0, path_max);
    realpath(opts.prefix, prefix);
    unsigned long int size = strlen(prefix) + 1;
    opts.prefix = malloc(size);
    memset((void *)opts.prefix, 0, size);
    memcpy((void *)opts.prefix, prefix, size);
  }

  clib_cache_init(CLIB_PACKAGE_CACHE_TIME);

  package_opts.skip_cache = 1;
  package_opts.prefix = opts.prefix;
  package_opts.global = 1;
  package_opts.force = opts.force;
  package_opts.token = opts.token;

#ifdef HAVE_PTHREADS
  package_opts.concurrency = opts.concurrency;
#endif

  clib_package_set_opts(package_opts);

  if (opts.prefix) {
    setenv("CLIB_PREFIX", opts.prefix, 1);
    setenv("PREFIX", opts.prefix, 1);
  }

  if (opts.force) {
    setenv("CLIB_FORCE", "1", 1);
  }

  char *slug = 0;

  if (0 == opts.tag && 0 != program.argv[0]) {
    opts.tag = program.argv[0];
  }

  if (0 == opts.slug) {
    slug = "clibs/clib";
  } else {
    slug = opts.slug;
  }

  int code = install_package(slug);

  curl_global_cleanup();
  clib_package_cleanup();

  command_free(&program);
  return code;
}
