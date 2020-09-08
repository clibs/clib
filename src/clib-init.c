//
// clib-init.c
//
// Copyright (c) 2012-2020 clib authors
// MIT licensed
//

#include "asprintf/asprintf.h"
#include "commander/commander.h"
#include "common/clib-package.h"
#include "debug/debug.h"
#include "fs/fs.h"
#include "logger/logger.h"
#include "parson/parson.h"
#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) ||               \
    defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(k, v, _) _putenv_s(k, v)
#define realpath(a, b) _fullpath(a, b, strlen(a))
#endif

debug_t debugger;

struct options {
  char *manifest;
  int verbose;
};

static struct options opts;

/**
 * Option setters.
 */

static void setopt_quiet(command_t *self) {
  opts.verbose = 0;
  debug(&debugger, "set quiet flag");
}

static void setopt_manifest_file(command_t *self) {
  opts.manifest = (char *)self->arg;
  debug(&debugger, "set manifest: %s", opts.manifest);
}

/**
 * Program.
 */

static char *find_basepath() {
  char cwd[4096] = {0};
  getcwd(cwd, 4096);
  char *walk = cwd + strlen(cwd);
  while (*(--walk) != '/')
    ;
  char *basepath = malloc((size_t)(walk - cwd));
  strncpy(basepath, walk + 1, (size_t)(walk - cwd));
  return basepath;
}

static void getinput(char *buffer, size_t s) {
  char *walk = buffer;
  int c = 0;
  while ((walk - s) != buffer && (c = fgetc(stdin)) && c != '\n' && c != 0) {
    *(walk++) = c;
  }
}

static void ask_for(JSON_Object *root, const char *key,
                    const char *default_value, const char *question) {
  static char buffer[512] = {0};
  memset(buffer, '\0', 512);
  printf("%s", question);
  getinput(buffer, 512);
  char *value = (char *)(strlen(buffer) > 0 ? buffer : default_value);
  json_object_set_string(root, key, value);
}

static inline size_t write_to_file(const char *manifest, const char *str,
                                   size_t length) {
  size_t wrote = 0;

  FILE *file = fopen(manifest, "w+");
  if (!file) {
    debug(&debugger, "Cannot open %s file.", manifest);
    return 0;
  }

  wrote = fwrite(str, sizeof(char), length, file);
  fclose(file);

  return length - wrote;
}

static int write_package_file(const char *manifest, JSON_Value *pkg) {
  int rc = 0;
  char *package = json_serialize_to_string_pretty(pkg);

  if (0 != write_to_file(manifest, package, strlen(package))) {
    logger_error("Failed to write to %s", manifest);
    rc = 1;
    goto e1;
  }

  debug(&debugger, "Wrote %s file.", manifest);

e1:
  json_free_serialized_string(package);

  return rc;
}

/**
 * Entry point.
 */

int main(int argc, char *argv[]) {
  int exit_code = 0;
  opts.verbose = 1;
  opts.manifest = "clib.json";

  debug_init(&debugger, "clib-init");

  command_t program;

  command_init(&program, "clib-init", CLIB_VERSION);

  program.usage = "[options]";

  command_option(&program, "-q", "--quiet", "disable verbose output",
                 setopt_quiet);
  command_option(&program, "-M", "--manifest <filename>",
                 "give a manifest of the manifest file. (default: clib.json)",
                 setopt_manifest_file);
  command_parse(&program, argc, argv);

  debug(&debugger, "%d arguments", program.argc);

  JSON_Value *json = json_value_init_object();
  JSON_Object *root = json_object(json);

  char *basepath = find_basepath();
  char *package_name = NULL;

  int rc = asprintf(&package_name, "package name (%s): ", basepath);
  if (-1 == rc) {
    logger_error("error", "asprintf() out of memory");
    goto end;
  }

  ask_for(root, "name", basepath, package_name);
  ask_for(root, "version", "0.1.0", "version (default: 0.1.0): ");

  exit_code = write_package_file(opts.manifest, json);

end:
  free(package_name);
  free(basepath);

  json_value_free(json);
  command_free(&program);

  return exit_code;
}
