
//
// clib-install.c
//
// Copyright (c) 2012-2014 clib authors
// MIT licensed
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "asprintf/asprintf.h"
#include "fs/fs.h"
#include "tempdir/tempdir.h"
#include "commander/commander.h"
#include "clib-package/clib-package.h"
#include "http-get/http-get.h"
#include "logger/logger.h"
#include "debug/debug.h"
#include "parson/parson.h"
#include "str-concat/str-concat.h"
#include "str-replace/str-replace.h"
#include "version.h"

debug_t debugger;

struct options {
  const char *dir;
  int verbose;
  int dev;
  int save;
  int savedev;
};

static struct options opts;

/**
 * Option setters.
 */

static void
setopt_dir(command_t *self) {
  opts.dir = (char *) self->arg;
  debug(&debugger, "set dir: %s", opts.dir);
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

/**
 * Install dependency packages at `pwd`.
 */
static int
install_local_packages() {
  if (-1 == fs_exists("./package.json")) {
    logger_error("error", "Missing package.json");
    return 1;
  }

  debug(&debugger, "reading local package.json");
  char *json = fs_read("./package.json");
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

#define E_FORMAT(...) ({      \
  rc = asprintf(__VA_ARGS__); \
  if (-1 == rc) goto cleanup; \
});

static int
executable(clib_package_t *pkg) {
  int rc;
  char *url = NULL;
  char *file = NULL;
  char *tarball = NULL;
  char *command = NULL;
  char *dir = NULL;
  char *deps = NULL;
  char *tmp = NULL;
  char *reponame = NULL;

  debug(&debugger, "install executable %s", pkg->repo);

  tmp = gettempdir();
  if (NULL == tmp) {
    logger_error("error", "gettempdir() out of memory");
    return -1;
  }

  if (!pkg->repo) {
    logger_error("error", "repo field required to install executable");
    return -1;
  }

  reponame = strrchr(pkg->repo, '/');
  if (reponame && *reponame != '\0') reponame++;
  else {
    logger_error("error", "malformed repo field, must be in the form user/pkg");
    return -1;
  }

  E_FORMAT(&url
    , "https://github.com/%s/archive/%s.tar.gz"
    , pkg->repo
    , pkg->version);
  E_FORMAT(&file, "%s-%s.tar.gz", reponame, pkg->version);
  E_FORMAT(&tarball, "%s/%s", tmp, file);
  rc = http_get_file(url, tarball);
  E_FORMAT(&command, "cd %s && gzip -dc %s | tar x", tmp, file);

  debug(&debugger, "download url: %s", url);
  debug(&debugger, "file: %s", file);
  debug(&debugger, "tarball: %s", tarball);
  debug(&debugger, "command: %s", command);

  // cheap untar
  rc = system(command);
  if (0 != rc) goto cleanup;

  E_FORMAT(&dir, "%s/%s-%s", tmp, reponame, pkg->version);
  debug(&debugger, "dir: %s", dir);

  if (pkg->dependencies) {
    E_FORMAT(&deps, "%s/deps", dir);
    debug(&debugger, "deps: %s", deps);
    rc = clib_package_install_dependencies(pkg, deps, opts.verbose);
    if (-1 == rc) goto cleanup;
  }

  free(command);
  command = NULL;

  E_FORMAT(&command, "cd %s && %s", dir, pkg->install);
  debug(&debugger, "command: %s", command);
  rc = system(command);

cleanup:
  free(tmp);
  free(dir);
  free(command);
  free(tarball);
  free(file);
  free(url);
  return rc;
}

#undef E_FORMAT

/**
 * Writes out a dependency to package.json
 */
static int
write_dependency(clib_package_t *pkg, char* prefix) {
  JSON_Value *packageJson = json_parse_file("package.json");
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
  int retCode = json_serialize_to_file_pretty(packageJson, "package.json");
  json_value_free(packageJson);
  return retCode;
}

/**
 * Save a dependency to package.json.
 */
static int
save_dependency(clib_package_t *pkg) {
  debug(&debugger, "saving dependency %s at %s", pkg->name, pkg->version);
  return write_dependency(pkg, "dependencies");
}

/**
 * Save a development dependency to package.json.
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
  int rc;

  clib_package_t *pkg = clib_package_new_from_slug(slug, opts.verbose);
  if (NULL == pkg) return -1;

  if (pkg->install) {
    rc = executable(pkg);
    goto check_save;
  }

  rc = clib_package_install(pkg, opts.dir, opts.verbose);
  if (0 == rc && opts.dev) {
    rc = clib_package_install_development(pkg, opts.dir, opts.verbose);
  }

check_save:
  if (opts.save) save_dependency(pkg);
  if (opts.savedev) save_dev_dependency(pkg);
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
    , "save dependency in package.json"
    , setopt_save);
  command_option(&program
      , "-D"
      , "--save-dev"
      , "save development dependency in package.json"
      , setopt_savedev);
  command_parse(&program, argc, argv);

  debug(&debugger, "%d arguments", program.argc);

  int code = 0 == program.argc
    ? install_local_packages()
    : install_packages(program.argc, program.argv);

  command_free(&program);
  return code;
}
