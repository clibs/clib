
//
// clib-install.c
//
// Copyright (c) 2012-2014 clib authors
// MIT licensed
//

#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "fs/fs.h"
#include "commander/commander.h"
#include "clib-package/clib-package.h"
#include "clib-cache/cache.h"
#include "http-get/http-get.h"
#include "logger/logger.h"
#include "debug/debug.h"
#include "parson/parson.h"
#include "str-concat/str-concat.h"
#include "str-replace/str-replace.h"
#include "version.h"
#include "clib-cache/cache.h"

#define CLIB_PACKAGE_CACHE_TIME 30 * 24 * 60 * 60

extern CURLSH *clib_package_curl_share;

debug_t debugger = { 0 };

struct options {
  const char *dir;
  char *prefix;
  int verbose;
  int dev;
  int save;
  int savedev;
  int force;
  int skip_cache;
};

static struct options opts = { 0 };

static const char *manifest_names[] = {
  "clib.json",
  "package.json",
  NULL
};

static clib_package_opts_t package_opts;

/**
 * Option setters.
 */

static void
setopt_dir(command_t *self) {
  opts.dir = (char *) self->arg;
  debug(&debugger, "set dir: %s", opts.dir);
}

static void
setopt_prefix(command_t *self) {
  opts.prefix = (char *) self->arg;
  debug(&debugger, "set prefix: %s", opts.prefix);
}

static void
setopt_quiet(command_t *self) {
  opts.verbose = 0;
  debug(&debugger, "set quiet flag");
}

static void
setopt_dev(command_t *self) {
  opts.dev = 1;
  debug(&debugger, "set development flag");
}

static void
setopt_save(command_t *self) {
  opts.save = 1;
  debug(&debugger, "set save flag");
}

static void
setopt_savedev(command_t *self) {
  opts.savedev = 1;
  debug(&debugger, "set savedev flag");
}

static void
setopt_force(command_t *self) {
  opts.force = 1;
  debug(&debugger, "set force flag");
}

static void
setopt_skip_cache(command_t *self) {
  opts.skip_cache = 1;
  debug(&debugger, "set skip cache flag");
}

static int
install_local_packages_with_package_name(const char *file) {
  if (-1 == fs_exists(file)) {
    logger_error("error", "Missing clib.json or package.json");
    return 1;
  }

  debug(&debugger, "reading local clib.json or package.json");
  char *json = fs_read(file);
  if (NULL == json) return 1;


  clib_package_t *pkg = clib_package_new(json, opts.verbose);
  if (NULL == pkg) goto e1;

  int rc = clib_package_install_dependencies(pkg, opts.dir, opts.verbose);
  if (-1 == rc) goto e2;

  if (opts.dev) {
    rc = clib_package_install_development(pkg, opts.dir, opts.verbose);
    if (-1 == rc) goto e2;
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
static int
install_local_packages() {
  const char *name = NULL;
  unsigned int i = 0;
  int rc = 0;

  do {
    name = manifest_names[i];
    rc = install_local_packages_with_package_name(name);
  } while (NULL != manifest_names[++i] && 0 != rc);

  return rc;
}

static int
write_dependency_with_package_name(clib_package_t *pkg, char* prefix, const char *file) {
  JSON_Value *packageJson = json_parse_file(file);
  JSON_Object *packageJsonObject = json_object(packageJson);
  JSON_Value *newDepSectionValue = NULL;

  if (NULL == packageJson || NULL == packageJsonObject) return 1;

  // If the dependency section doesn't exist then create it
  JSON_Object *depSection = json_object_dotget_object(packageJsonObject, prefix);
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
 * Writes out a dependency to clib.json or package.json
 */
static int
write_dependency(clib_package_t *pkg, char* prefix) {
  const char *name = NULL;
  unsigned int i = 0;
  int rc = 0;

  do {
    name = manifest_names[i];
    rc = write_dependency_with_package_name(pkg, prefix, name);
  } while (NULL != manifest_names[++i] && 0 != rc);

  return rc;
}

/**
 * Save a dependency to clib.json or package.json.
 */
static int
save_dependency(clib_package_t *pkg) {
  debug(&debugger, "saving dependency %s at %s", pkg->name, pkg->version);
  return write_dependency(pkg, "dependencies");
}

/**
 * Save a development dependency to clib.json or package.json.
 */
static int
save_dev_dependency(clib_package_t *pkg) {
  debug(&debugger, "saving dev dependency %s at %s", pkg->name, pkg->version);
  return write_dependency(pkg, "development");
}

/**
 * Create and install a package from `slug`.
 */

static int
install_package(const char *slug) {
  clib_package_t *pkg = clib_package_new_from_slug(slug, opts.verbose);
  int rc;

  if (NULL == pkg) return -1;

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

  if (0 != strcmp(slug, pkg->repo)) {
    pkg->repo = strdup(slug);
  }
save:
  if (opts.save) save_dependency(pkg);
  if (opts.savedev) save_dev_dependency(pkg);

cleanup:
  clib_package_free(pkg);
  return rc;
}

/**
 * Install the given `pkgs`.
 */

static int
install_packages(int n, char *pkgs[]) {
  for (int i = 0; i < n; i++) {
    debug(&debugger, "install %s (%d)", pkgs[i], i);
    if (-1 == install_package(pkgs[i])) return 1;
  }
  return 0;
}

/**
 * Entry point.
 */

int
main(int argc, char *argv[]) {
#ifdef _WIN32
  opts.dir = ".\\deps";
#else
  opts.dir = "./deps";
#endif
  opts.verbose = 1;
  opts.dev = 0;

  debug_init(&debugger, "clib-install");

  //30 days expiration
  clib_cache_init(CLIB_PACKAGE_CACHE_TIME);

  command_t program;

  command_init(&program
    , "clib-install"
    , CLIB_VERSION);

  program.usage = "[options] [name ...]";

  command_option(&program
    , "-o"
    , "--out <dir>"
    , "change the output directory [deps]"
    , setopt_dir);
  command_option(&program
    , "-P"
    , "--prefix <dir>"
    , "change the prefix directory (usually '/usr/local')"
    , setopt_prefix);
  command_option(&program
    , "-q"
    , "--quiet"
    , "disable verbose output"
    , setopt_quiet);
  command_option(&program
    , "-d"
    , "--dev"
    , "install development dependencies"
    , setopt_dev);
  command_option(&program
    , "-S"
    , "--save"
    , "save dependency in clib.json or package.json"
    , setopt_save);
  command_option(&program
      , "-D"
      , "--save-dev"
      , "save development dependency in clib.json or package.json"
      , setopt_savedev);
  command_option(&program
      , "-f"
      , "--force"
      , "force the action of something, like overwriting a file"
      , setopt_force);
  command_option(&program
      , "-c"
      , "--skip-cache"
      , "skip cache when installing"
      , setopt_skip_cache);
  command_parse(&program, argc, argv);

  clib_package_set_opts(package_opts);

  debug(&debugger, "%d arguments", program.argc);

  if (0 != curl_global_init(CURL_GLOBAL_ALL)) {
    logger_error("error", "Failed to initialize cURL");
  }

  clib_package_set_opts((clib_package_opts_t) {
    .skip_cache = opts.skip_cache,
    .prefix = opts.prefix,
    .force = opts.force
  });

  if (1 != opts.skip_cache) {
    clib_cache_init(time(0));
  }

  int code = 0 == program.argc
    ? install_local_packages()
    : install_packages(program.argc, program.argv);

  curl_global_cleanup();

  command_free(&program);
  return code;
}
