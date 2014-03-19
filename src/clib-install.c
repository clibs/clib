
//
// clib-install.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fs/fs.h"
#include "commander/commander.h"
#include "clib-package/clib-package.h"
#include "http-get/http-get.h"
#include "logger/logger.h"
#include "version.h"

struct options {
  const char *dir;
  int verbose;
  int dev;
};

static struct options opts;

/**
 * Option setters.
 */

static void
setopt_dir(command_t *self) {
  opts.dir = (char *) self->arg;
}

static void
setopt_quite(command_t *self) {
  opts.verbose = 0;
}

static void
setopt_dev(command_t *self) {
  opts.dev = 1;
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

static int
executable(clib_package_t *pkg) {
  int rc;

  char *url = malloc(256);
  if (NULL == url) goto fail;
  sprintf(url
    , "https://github.com/%s/%s/archive/%s.tar.gz"
    , pkg->author
    , pkg->name
    , pkg->version);

  char *file = malloc(256);
  if (NULL == file) goto e1;
  sprintf(file
    , "%s-%s.tar.gz"
    , pkg->name
    , pkg->version);

  char *tarball = malloc(256);
  if (NULL == tarball) goto e2;
  sprintf(tarball, "/tmp/%s", file);

  rc = http_get_file(url, tarball);
  if (-1 == rc) goto e3;

  char *command = malloc(512);
  if (NULL == command) goto e3;

  // cheap untar
  sprintf(command
    , "cd /tmp && tar -xf %s"
    , file);
  rc = system(command);
  if (0 != rc) goto e4;

  char *dir = malloc(256);
  if (NULL == dir) goto e4;
  sprintf(dir
    , "/tmp/%s-%s"
    , pkg->name
    , pkg->version);

  if (pkg->dependencies) {
    char *deps = malloc(strlen(dir) + 6);
    if (deps) {
      sprintf(deps, "%s/deps", dir);
      rc = clib_package_install_dependencies(pkg, deps, opts.verbose);
      free(deps);
      if (-1 == rc) goto e5;
    } else goto e5;
  }

  // cheap install
  sprintf(command
    , "cd %s && %s"
    , dir
    , pkg->install);
  rc = system(command);
  free(dir);
  free(command);
  free(tarball);
  free(file);
  free(url);
  return 0;

e5: free(dir);
e4: free(command);
e3: free(tarball);
e2: free(file);
e1: free(url);
fail:
  return -1;
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
    goto done;
  }

  rc = clib_package_install(pkg, opts.dir, opts.verbose);
  if (0 == rc && opts.dev) {
    rc = clib_package_install_development(pkg, opts.dir, opts.verbose);
  }

done:
  clib_package_free(pkg);
  return rc;
}

/**
 * Install the given `pkgs`.
 */

static int
install_packages(int n, char *pkgs[]) {
  for (int i = 0; i < n; i++) {
    if (-1 == install_package(pkgs[i])) return 1;
  }
  return 0;
}

/**
 * Entry point.
 */

int
main(int argc, char *argv[]) {
  opts.dir = "./deps";
  opts.verbose = 1;
  opts.dev = 0;

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
    , "--quite"
    , "disable verbose output"
    , setopt_quite);
  command_option(&program
    , "-d"
    , "--dev"
    , "install development dependencies"
    , setopt_dev);
  command_parse(&program, argc, argv);

  int code = 0 == program.argc
    ? install_local_packages()
    : install_packages(program.argc, program.argv);

  command_free(&program);
  return code;
}
