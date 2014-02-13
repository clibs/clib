
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
  "";

int
main(int argc, char **argv) {
  if (NULL == argv[1]
   || 0 == strncmp(argv[1], "-h", 2)
   || 0 == strncmp(argv[1], "--help", 6)) {
    printf("%s\n", usage);
    return 0;
  }

  if (0 == strncmp(argv[1], "-v", 2)
   || 0 == strncmp(argv[1], "--version", 9)) {
    printf("%s\n", CLIB_VERSION);
    return 0;
  }

  if (0 == strncmp(argv[1], "--", 2)) {
    fprintf(stderr, "Unknown option: \"%s\"\n", argv[1]);
    return 1;
  }

  char *pcmd = strdup(argv[1]);
  if (NULL == pcmd) {
    fprintf(stderr, "Memory allocation failure\n");
    return 1;
  }
  char *cmd = trim(pcmd);

  // additional arguments to pass to subcommand
  char *args = NULL;
  if (argc >= 3) {
    args = str_flatten(argv, 2, argc);
    if (NULL == args) goto e1;
  }

  char *command = malloc(1024);
  if (NULL == command) goto e2;
#ifdef _WIN32
  sprintf(command, "clib-%s.exe", cmd);
#else
  sprintf(command, "clib-%s", cmd);
#endif

  char *bin = which(command);
  if (NULL == bin) {
    fprintf(stderr, "Unsupported command \"%s\".\n", cmd);
    goto e3;
  }

  if (args) {
    sprintf(command, "%s %s", bin, args);
  } else {
    strcpy(command, bin);
  }

  int code = system(command);

  free(pcmd);
  if (args) free(args);
  free(command);
  free(bin);

  return code;

e3:
  free(bin);
e2:
  if (args) free(args);
  free(command);
e1:
  free(pcmd);

  return 1;
}
