
//
// clib.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "trim/trim.h"
#include "which/which.h"
#include "str-flatten/str-flatten.h"
#include "str-copy/str-copy.h"
#include "util.h"
#include "version.h"

static const char *usage =
  "\n"
  "  clib <command> [options]\n"
  "\n"
  "  Options:\n"
  "\n"
  "    -h, --help     Output this message\n"
  "    -v, --version  Output version information\n"
  "\n"
  "  Commands:\n"
  "\n"
  "    install [name...]  Install one or more packages\n"
  "    search [query]     Search for packages\n"
  "    help <cmd>         Display help for cmd\n"
  "";

#define format(...) ({                               \
  if (-1 == asprintf(__VA_ARGS__)) {                 \
    rc = 1;                                          \
    fprintf(stderr, "Memory allocation failure\n");  \
    goto cleanup;                                    \
  }                                                  \
})

int
main(int argc, const char **argv) {
  char *cmd = NULL;
  char *args = NULL;
  char *command = NULL;
  char *command_with_args = NULL;
  char *bin = NULL;
  int rc = 1;

  // usage
  if (NULL == argv[1]
   || 0 == strncmp(argv[1], "-h", 2)
   || 0 == strncmp(argv[1], "--help", 6)) {
    printf("%s\n", usage);
    return 0;
  }

  // version
  if (0 == strncmp(argv[1], "-v", 2)
   || 0 == strncmp(argv[1], "--version", 9)) {
    printf("%s\n", CLIB_VERSION);
    return 0;
  }

  // unknown
  if (0 == strncmp(argv[1], "--", 2)) {
    fprintf(stderr, "Unknown option: \"%s\"\n", argv[1]);
    return 1;
  }

  // sub-command
  cmd = str_copy(argv[1]);
  if (NULL == cmd) {
    fprintf(stderr, "Memory allocation failure\n");
    return 1;
  }
  cmd = trim(cmd);

  if (0 == strcmp(cmd, "help")) {
    if (argc >= 3) {
      free(cmd);
      cmd = str_copy(argv[2]);
      args = str_copy("--help");
    } else {
      fprintf(stderr, "Help command required.\n");
      goto cleanup;
    }
  } else {
    if (argc >= 3) {
      args = str_flatten(argv, 2, argc);
      if (NULL == args) goto cleanup;
    }
  }

#ifdef _WIN32
  format(&command, "clib-%s.exe", cmd);
#else
  format(&command, "clib-%s", cmd);
#endif

  bin = which(command);
  if (NULL == bin) {
    fprintf(stderr, "Unsupported command \"%s\"\n", cmd);
    goto cleanup;
  }

#ifdef _WIN32
  for (char *p = bin; *p; p++)
    if (*p == '/') *p = '\\';
#endif

  if (args) {
    format(&command_with_args, "%s %s", bin, args);
  } else {
    format(&command_with_args, "%s", bin);
  }

  rc = system(command_with_args);
  if (rc > 255) rc = 1;

cleanup:
  free(cmd);
  free(args);
  free(command);
  free(command_with_args);
  free(bin);
  return rc;
}
