//
// clib-init.c
//
// Copyright (c) 2012-2019 clib authors
// MIT licensed
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "console-colors/console-colors.h"
#include "clib-package/clib-package.h"
#include "commander/commander.h"
#include "asprintf/asprintf.h"
#include "logger/logger.h"
#include "parson/parson.h"
#include "debug/debug.h"
#include "case/case.h"
#include "version.h"
#include "fs/fs.h"

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(k, v, _) _putenv_s(k, v)
#define realpath(a, b) _fullpath(a, b, strlen(a))
#endif

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
void ask_for(JSON_Object* root,
             const char* key,
             const char* default_value,
             const char* question) {
  char buffer[BUFSIZ] = { 0 };

  if (0 != default_value && 0 != strlen(default_value)) {
    printf("%s: (%s) ", question, default_value);
  } else {
    printf("%s: ", question);
  }

  getinput(buffer, BUFSIZ);
  char* value = (char*)(strlen(buffer) > 0 ? buffer : default_value);

  if ( 0 != value && 0 != strlen(value)) {
    json_object_set_string(root, key, value);
  }
}

static inline size_t
write_to_file(const char* manifest, const char* str, size_t length) {
  size_t wrote = 0;

  FILE* file = fopen(manifest, "w+");
  if (!file) {
    debug(&debugger, "Cannot open %s file.", manifest);
    return 0;
  }

  wrote = fwrite(str, sizeof(char), length, file);
  fclose(file);

  return wrote == length ? 0 : -1;
}

static int
write_package_file(const char* manifest, JSON_Value* pkg) {
  int rc = 0;
  char* package = json_serialize_to_string_pretty(pkg);
  char confirmation[BUFSIZ] = { 0 };

  printf("Will write file %s: \n\n", manifest);
  printf("%s\n\n", package);

  printf("Is this OK? (Yes): ");
  getinput(confirmation, BUFSIZ);

  if (0 == strlen(confirmation) || 'y' == case_lower(confirmation)[0]) {
    if (0 != (rc = write_to_file(manifest, package, strlen(package)))) {
      logger_error("Failed to write to %s", manifest);
      goto e1;
    }
    debug(&debugger, "Wrote %s file.", manifest);
  } else {
    rc = 1;
  }

 e1:
  json_free_serialized_string(package);

  return rc;
}

/**
 * Entry point.
 */

int
main(int argc, char *argv[]) {
  int exit_code = 0;
  opts.verbose = 1;
  opts.manifest = "clib.json";

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
                 , "give a manifest of the manifest file. (default: clib.json)"
                 , setopt_manifest_file);
  command_parse(&program, argc, argv);

  debug(&debugger, "%d arguments", program.argc);

  JSON_Object *root = NULL;
  JSON_Value *json = NULL;

  if (0 == fs_exists(opts.manifest)) {
    json = json_parse_string(fs_read(opts.manifest));
    root = json_value_get_object(json);
  } else {
    json = json_value_init_object();
    root = json_object(json);
  }

  char* basepath = find_basepath();
  char* package_name = NULL;

  int rc = asprintf(&package_name, "package name (%s): ", basepath);
  if (-1 == rc) {
    logger_error("error", "asprintf() out of memory");
    goto end;
  }

  struct {
    const char *description;
    const char *version;
    const char *name;
    const char *repo;
    const char *license;
    // build
    const char *configure;
    const char *makefile;
    const char *install;
    const char *prefix;
  } defaults = {
    .description = json_object_get_string(root, "description"),
    .version = json_object_get_string(root, "version"),
    .name = json_object_get_string(root, "name"),
    .repo = json_object_get_string(root, "repo"),
    .license = json_object_get_string(root, "license"),

    .configure = json_object_get_string(root, "configure"),
    .makefile = json_object_get_string(root, "makefile"),
    .install = json_object_get_string(root, "install"),
    .prefix = json_object_get_string(root, "prefix"),
  };

  printf(
      "This command line utility will walk you through the creation\n"
      "of a '%s' file for '%s'.\n"
      "\n"
      "Install a package dependency after creating this file by running:\n"
      "`clib install <user>/<repo> --save`\n"
      "\n"
      "You can search for packages by using `clib search [query]`\n"
      "\n"
      "Other useful commands like `clib configure` and `clib build` allow\n"
      "you to configure and build your package or dependency packages from\n"
      "the `clib` command line interface.\n"
      "\n"
      "Try the `--help` flag with any `clib` command to learn more about its\n"
      "usage.\n"
      "\n"
      "To cancel or exit this prompt at any time, press ^C to quit.\n"
      "\n",
      opts.manifest,
      basepath);

#define default(key, value) 0 != defaults.key ? defaults.key : value
  ask_for(root, "name", default(name, basepath), "Package Name");
  ask_for(root, "repo", default(repo, 0), "Repository");
  ask_for(root, "version", default(version, "0.0.1"), "Version");
  ask_for(root, "description", default(description, 0), "Description");
  ask_for(root, "license", default(license, "MIT"), "License Type");

  ask_for(root, "configure", default(configure, 0), "Configure Script");
  ask_for(root, "makefile", default(makefile, 0), "Makefile");
  ask_for(root, "install", default(install, 0), "Install Script");
  ask_for(root, "prefix", default(prefix, "./"), "PREFIX Path");
#undef default

  exit_code = write_package_file(opts.manifest, json);

 end:
  free(package_name);
  free(basepath);

  json_value_free(json);
  command_free(&program);

  return exit_code;
}
