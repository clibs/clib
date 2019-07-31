//
// clib-init.c
//
// Copyright (c) 2012-2014 clib authors
// MIT licensed
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "fs/fs.h"
#include "asprintf/asprintf.h"
#include "commander/commander.h"
#include "clib-package/clib-package.h"
#include "logger/logger.h"
#include "debug/debug.h"
#include "parson/parson.h"
#include "version.h"

debug_t debugger;

struct options {
  char* manifest;
  int verbose;
};

static struct options opts;

/**
 * Option setters.
 */

static void
setopt_quiet(command_t *self) {
  opts.verbose = 0;
  debug(&debugger, "set quiet flag");
}

static void
setopt_manifest_file(command_t *self) {
  opts.manifest = (char *) self->arg;
  debug(&debugger, "set manifest: %s", opts.manifest);
}

/**
 * Program.
 */

static
char* find_basepath() {
  char cwd[512] = { 0 };
  getcwd(cwd, 512);
  debug(&debugger, "%s cwd", cwd);

  char* walk = cwd + strlen(cwd);
  while(*(--walk) != '/');
  char* basepath = malloc((size_t)(walk - cwd));
  strncpy(basepath, walk + 1, (size_t)(walk - cwd));
  return basepath;
}

static
void getinput(char* buffer, size_t s) {
  char* walk = buffer;
  int c = 0;
  while ((walk - s) != buffer &&
         (c = fgetc(stdin)) &&
         c != '\n' && c != 0) {
    *(walk++) = c;
  }
}

static
void ask_for(JSON_Object* root, const char* key,
             const char* default_value,
             const char* question) {
  static char buffer[512] = { 0 };
  memset(buffer, '\0', 512);
  printf(question);
  getinput(buffer, 512);
  char* value = (char*)(strlen(buffer) > 0 ? buffer : default_value);
  json_object_set_string(root, key, value);
}

static int
write_package_file(const char* manifest, JSON_Value* pkg) {
  const char* package = json_serialize_to_string_pretty(pkg);
  FILE* package_file = fopen(manifest, "w+");
  if (!package_file) {
    debug(&debugger, "Cannot open %s file.", manifest);
    return 1;
  }

  size_t len = strlen(package);
  const size_t wrote = fwrite(package, sizeof(char), len, package_file);

  char* package_name = NULL;
  int rc = asprintf(&package_name, "Wrote %s file.", manifest);
  if (-1 == rc) {
    logger_error("error", "asprintf() out of memory");
    free(package_name);
    return 1;
  }
  debug(&debugger, package_name);

  fclose(package_file);

  return wrote == len ? 0 : 1;
}

/**
 * Entry point.
 */

int
main(int argc, char *argv[]) {
  opts.verbose = 1;
  opts.manifest = "package.json";

  debug_init(&debugger, "clib-init");

  command_t program;

  command_init(&program
    , "clib-init"
    , CLIB_VERSION);

  program.usage = "[options]";

  command_option(&program
                 , "-q"
                 , "--quiet"
                 , "disable verbose output"
                 , setopt_quiet);
  command_option(&program
                 , ""
                 , "--manifest <filename>"
                 , "give a manifest of the manifest file. (default: package.json)"
                 , setopt_manifest_file);
  command_parse(&program, argc, argv);

  debug(&debugger, "%d arguments", program.argc);

  JSON_Value* json = json_value_init_object();
  JSON_Object* root = json_object(json);

  char* basepath = find_basepath();
  char* package_name = NULL;

  int rc = asprintf(&package_name, "package name (%s): ", basepath);
  if (-1 == rc) {
    logger_error("error", "asprintf() out of memory");
    free(package_name);
    goto end;
  }

  ask_for(root, "name", basepath, package_name);
  ask_for(root, "version", "0.0.1", "version (default: 0.0.1): ");

  int code = write_package_file(opts.manifest, json);

 end:
  free(basepath);
  command_free(&program);

  return code;
}
