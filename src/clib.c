//
// clib.c
//
// Copyright (c) 2012-2020 clib authors
// MIT licensed
//

#include "asprintf/asprintf.h"
#include "common/clib-cache.h"
#include "common/clib-release-info.h"
#include "debug/debug.h"
#include "fs/fs.h"
#include "http-get/http-get.h"
#include "logger/logger.h"
#include "parson/parson.h"
#include "path-join/path-join.h"
#include "semver/semver.h"
#include "str-flatten/str-flatten.h"
#include "strdup/strdup.h"
#include "trim/trim.h"
#include "version.h"
#include "which/which.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) ||               \
    defined(__MINGW64__) || defined(__CYGWIN__)
#define setenv(k, v, _) _putenv_s(k, v)
#define realpath(a, b) _fullpath(a, b, strlen(a))
#endif

#define LATEST_RELEASE_ENDPOINT                                                \
  "https://api.github.com/repos/clibs/clib/releases/latest"
#define RELEASE_NOTIFICATION_EXPIRATION 3 * 24 * 60 * 60 // 3 days

debug_t debugger;

static const char *usage =
    "\n"
    "  clib <command> [options]\n"
    "\n"
    "  Options:\n"
    "\n"
    "    -h, --help     Output this message\n"
    "    -V, --version  Output version information\n"
    "\n"
    "  Commands:\n"
    "\n"
    "    init                 Start a new project\n"
    "    i, install [name...] Install one or more packages\n"
    "    up, update [name...] Update one or more packages\n"
    "    uninstall [name...]  Uninstall executables\n"
    "    upgrade [version]    Upgrade clib to a specified or latest version\n"
    "    configure [name...]  Configure one or more packages\n"
    "    build [name...]      Build one or more packages\n"
    "    search [query]       Search for packages\n"
    "    help <cmd>           Display help for cmd\n"
    "";

#define format(...)                                                            \
  ({                                                                           \
    if (-1 == asprintf(__VA_ARGS__)) {                                         \
      rc = 1;                                                                  \
      fprintf(stderr, "Memory allocation failure\n");                          \
      goto cleanup;                                                            \
    }                                                                          \
  })

static bool should_check_release(const char *path) {
  fs_stats *stat = fs_stat(path);

  if (!stat) {
    return true;
  }

  time_t modified = stat->st_mtime;
  time_t now = time(NULL);
  free(stat);

  return now - modified >= RELEASE_NOTIFICATION_EXPIRATION;
}

static void compare_versions(const char *marker_file_path) {
  if (!marker_file_path)
    return;

  const char *latest_tag = clib_release_get_latest_tag();

  if (!latest_tag)
    return;

  semver_t current_version;

  if (0 != semver_parse(CLIB_VERSION, &current_version)) {
    debug(&debugger, "Unable to parse the current version as a semver_t: %s",
          CLIB_VERSION);
    return;
  }

  semver_t latest_version;

  if (0 != semver_parse(latest_tag, &latest_version)) {
    debug(&debugger, "Unable to parse the latest tag as a semver_t: %s",
          latest_tag);
    semver_free(&current_version);
    return;
  }

  if (semver_satisfies(current_version, latest_version, "<")) {
    logger_info("info",
                "You are using clib %s, a new version is avalable. You can "
                "upgrade with the following command: clib upgrade --tag %s",
                CLIB_VERSION, latest_tag);
  } else {
    debug(&debugger, "No newer version found. Current: %s Latest: %s",
          CLIB_VERSION, latest_tag);
  }

  semver_free(&current_version);
  semver_free(&latest_version);

  free((void *)latest_tag);
}

static void notify_new_release(void) {
  const char *marker_file_path =
      path_join(clib_cache_meta_dir(), "release-notification-checked");

  if (!marker_file_path) {
    debug(&debugger,
          "Unable to retrieve release notification marker file path");
    return;
  }

  if (!should_check_release(marker_file_path)) {
    debug(&debugger, "No need to check for new release yet");
    goto cleanup;
  }

  compare_versions(marker_file_path);
  fs_write(marker_file_path, " ");

cleanup:
  free((void *)marker_file_path);
}

static void warn_deprecated_sub_command(const char *cmd) {
  const char *allowed[] = {"build",   "configure", "init",
                           "install", "search",    "update",
                           "upgrade", "uninstall", NULL};

  int i = 0;

  do {
    if (0 == strcmp(allowed[i], cmd)) {
      return;
    }
  } while (NULL != allowed[++i]);

  logger_warn("deprecated", "Invoking external clib-* executables as "
                            "sub-commands will be removed in 3.0");
}

int main(int argc, const char **argv) {

  char *cmd = NULL;
  char *args = NULL;
  char *command = NULL;
  char *command_with_args = NULL;
  char *bin = NULL;
  int rc = 1;

  debug_init(&debugger, "clib");

  clib_cache_meta_init();

  notify_new_release();

  // usage
  if (NULL == argv[1] || 0 == strncmp(argv[1], "-h", 2) ||
      0 == strncmp(argv[1], "--help", 6)) {
    printf("%s\n", usage);
    return 0;
  }

  // version
  if (0 == strncmp(argv[1], "-V", 2) || 0 == strncmp(argv[1], "--version", 9)) {
    printf("%s\n", CLIB_VERSION);
    return 0;
  }

  // unknown
  if (0 == strncmp(argv[1], "--", 2)) {
    fprintf(stderr, "Unknown option: \"%s\"\n", argv[1]);
    return 1;
  }

  // sub-command
  cmd = strdup(argv[1]);
  if (NULL == cmd) {
    fprintf(stderr, "Memory allocation failure\n");
    return 1;
  }
  cmd = trim(cmd);

  if (0 == strcmp(cmd, "help")) {
    if (argc >= 3) {
      free(cmd);
      cmd = strdup(argv[2]);
      args = strdup("--help");
    } else {
      fprintf(stderr, "Help command required.\n");
      goto cleanup;
    }
  } else {
    if (argc >= 3) {
      args = str_flatten(argv, 2, argc);
      if (NULL == args)
        goto cleanup;
    }
  }
  debug(&debugger, "args: %s", args);

  // aliases
  cmd = strcmp(cmd, "i") == 0 ? strdup("install") : cmd;
  cmd = strcmp(cmd, "up") == 0 ? strdup("update") : cmd;

  warn_deprecated_sub_command(cmd);

#ifdef _WIN32
  format(&command, "clib-%s.exe", cmd);
#else
  format(&command, "clib-%s", cmd);
#endif
  debug(&debugger, "command '%s'", cmd);

  bin = which(command);
  if (NULL == bin) {
    fprintf(stderr, "Unsupported command \"%s\"\n", cmd);
    goto cleanup;
  }

#ifdef _WIN32
  for (char *p = bin; *p; p++)
    if (*p == '/')
      *p = '\\';
#endif

  if (args) {
    format(&command_with_args, "%s %s", bin, args);
  } else {
    format(&command_with_args, "%s", bin);
  }

  debug(&debugger, "exec: %s", command_with_args);

  rc = system(command_with_args);
  debug(&debugger, "returned %d", rc);
  if (rc > 255)
    rc = 1;

cleanup:
  free(cmd);
  free(args);
  free(command);
  free(command_with_args);
  free(bin);
  return rc;
}
