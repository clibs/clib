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
#include "version.h"
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
#define MAX_THREADS 12
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) ||               \
    defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(k, v, _) _putenv_s(k, v)
#define realpath(a, b) _fullpath(a, b, strlen(a))
#endif

extern CURLSH *clib_package_curl_share;

debug_t debugger = {0};

struct options {
  const char *dir;
  char *prefix;
  char *token;
  int verbose;
  int dev;
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

static void setopt_dir(command_t *self) {
  opts.dir = (char *)self->arg;
  debug(&debugger, "set dir: %s", opts.dir);
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

static void setopt_dev(command_t *self) {
  opts.dev = 1;
  debug(&debugger, "set development flag");
}

#ifdef HAVE_PTHREADS
static void setopt_concurrency(command_t *self) {
  if (self->arg) {
    opts.concurrency = atol(self->arg);
    debug(&debugger, "set concurrency: %lu", opts.concurrency);
  }
}
#endif

static int install_local_packages_with_package_name(const char *file) {
  if (-1 == fs_exists(file)) {
    logger_error("error", "Missing clib.json or package.json");
    return 1;
  }

  debug(&debugger, "reading local clib.json or package.json");
  char *json = fs_read(file);
  if (NULL == json)
    return 1;

  clib_package_t *pkg = clib_package_new(json, opts.verbose);
  if (NULL == pkg)
    goto e1;

  if (pkg->prefix) {
    setenv("PREFIX", pkg->prefix, 1);
  }

  int rc = clib_package_install_dependencies(pkg, opts.dir, opts.verbose);
  if (-1 == rc)
    goto e2;

  if (opts.dev) {
    rc = clib_package_install_development(pkg, opts.dir, opts.verbose);
    if (-1 == rc)
      goto e2;
  }

  free(json);
  clib_package_free(pkg);
  return 0;

e2:
  clib_package_free(pkg);
e1:
  free(json);
  return 1;
}

/**
 * Install dependency packages at `pwd`.
 */
static int install_local_packages() {
  const char *name = NULL;
  unsigned int i = 0;
  int rc = 0;

  do {
    name = manifest_names[i];
    rc = install_local_packages_with_package_name(name);
  } while (NULL != manifest_names[++i] && 0 != rc);

  return rc;
}

static int write_dependency_with_package_name(clib_package_t *pkg, char *prefix,
                                              const char *file) {
  JSON_Value *packageJson = json_parse_file(file);
  JSON_Object *packageJsonObject = json_object(packageJson);
  JSON_Value *newDepSectionValue = NULL;

  if (NULL == packageJson || NULL == packageJsonObject)
    return 1;

  // If the dependency section doesn't exist then create it
  JSON_Object *depSection =
      json_object_dotget_object(packageJsonObject, prefix);
  if (NULL == depSection) {
    newDepSectionValue = json_value_init_object();
    depSection = json_value_get_object(newDepSectionValue);
    json_object_set_value(packageJsonObject, prefix, newDepSectionValue);
  }

  // Add the dependency to the dependency section
  json_object_set_string(depSection, pkg->repo, pkg->version);

  // Flush package.json
  int rc = json_serialize_to_file_pretty(packageJson, file);
  json_value_free(packageJson);
  return rc;
}

/**
 * Create and install a package from `slug`.
 */

static int install_package(const char *slug) {
  clib_package_t *pkg = NULL;
  int rc;

#ifdef PATH_MAX
  long path_max = PATH_MAX;
#elif defined(_PC_PATH_MAX)
  long path_max = pathconf(slug, _PC_PATH_MAX);
#else
  long path_max = 4096;
#endif

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

  if ('.' == slug[0]) {
    if (1 == strlen(slug) || ('/' == slug[1] && 2 == strlen(slug))) {
      char dir[path_max];
      realpath(slug, dir);
      slug = dir;
      return install_local_packages();
    }
  }

  if (0 == fs_exists(slug)) {
    fs_stats *stats = fs_stat(slug);
    if (NULL != stats && (S_IFREG == (stats->st_mode & S_IFMT)
#if defined(__unix__) || defined(__linux__) || defined(_POSIX_VERSION)
                          || S_IFLNK == (stats->st_mode & S_IFMT)
#endif
                              )) {
      free(stats);
      return install_local_packages_with_package_name(slug);
    }

    if (stats) {
      free(stats);
    }
  }

  if (!pkg) {
    pkg = clib_package_new_from_slug(slug, opts.verbose);
  }

  if (NULL == pkg)
    return -1;

  if (root_package && root_package->prefix) {
    package_opts.prefix = root_package->prefix;
    clib_package_set_opts(package_opts);
  }

  rc = clib_package_install(pkg, opts.dir, opts.verbose);
  if (0 != rc) {
    goto cleanup;
  }

  if (0 == rc && opts.dev) {
    rc = clib_package_install_development(pkg, opts.dir, opts.verbose);
    if (0 != rc) {
      goto cleanup;
    }
  }

  if (0 == pkg->repo || 0 != strcmp(slug, pkg->repo)) {
    pkg->repo = strdup(slug);
  }

cleanup:
  clib_package_free(pkg);
  return rc;
}

/**
 * Install the given `pkgs`.
 */

static int install_packages(int n, char *pkgs[]) {
  for (int i = 0; i < n; i++) {
    debug(&debugger, "install %s (%d)", pkgs[i], i);
    if (-1 == install_package(pkgs[i]))
      return 1;
  }
  return 0;
}

/**
 * Entry point.
 */

int main(int argc, char *argv[]) {
#ifdef _WIN32
  opts.dir = ".\\deps";
#else
  opts.dir = "./deps";
#endif
  opts.verbose = 1;
  opts.dev = 0;

#ifdef PATH_MAX
  long path_max = PATH_MAX;
#elif defined(_PC_PATH_MAX)
  long path_max = pathconf(opts.dir, _PC_PATH_MAX);
#else
  long path_max = 4096;
#endif

  debug_init(&debugger, "clib-update");

  // 30 days expiration
  clib_cache_init(CLIB_PACKAGE_CACHE_TIME);

  command_t program;

  command_init(&program, "clib-update", CLIB_VERSION);

  program.usage = "[options] [name ...]";

  command_option(&program, "-o", "--out <dir>",
                 "change the output directory [deps]", setopt_dir);
  command_option(&program, "-P", "--prefix <dir>",
                 "change the prefix directory (usually '/usr/local')",
                 setopt_prefix);
  command_option(&program, "-q", "--quiet", "disable verbose output",
                 setopt_quiet);
  command_option(&program, "-d", "--dev", "install development dependencies",
                 setopt_dev);
  command_option(&program, "-t", "--token <token>",
                 "Access token used to read private content", setopt_token);
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
  package_opts.global = 0;
  package_opts.force = 1;
  package_opts.token = opts.token;

#ifdef HAVE_PTHREADS
  package_opts.concurrency = opts.concurrency;
#endif

  clib_package_set_opts(package_opts);

  if (opts.prefix) {
    setenv("CLIB_PREFIX", opts.prefix, 1);
    setenv("PREFIX", opts.prefix, 1);
  }

  setenv("CLIB_FORCE", "1", 1);

  int code = 0 == program.argc ? install_local_packages()
                               : install_packages(program.argc, program.argv);

  curl_global_cleanup();
  clib_package_cleanup();

  command_free(&program);
  return code;
}
