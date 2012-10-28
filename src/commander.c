
//
// commander.c
//
// Copyright (c) 2012 TJ Holowaychuk <tj@vision-media.ca>
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "commander.h"

/*
 * Output error and exit.
 */

static void
error(char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

/*
 * Output command version.
 */

static void
command_version(command_t *self) {
  printf("%s\n", self->version);
  exit(0);
}

/*
 * Output command help.
 */

void
command_help(command_t *self) {
  printf("\n");
  printf("  Usage: %s %s\n", self->name, self->usage);
  printf("\n");
  printf("  Options:\n");
  printf("\n");
  for (int i = 0; i < self->option_count; ++i) {
    command_option_t *option = &self->options[i];
    printf("    %s, %-25s %s\n"
      , option->small
      , option->large_with_arg
      , option->description);
  }
  printf("\n");
  exit(0);
}

/*
 * Initialize with program `name` and `version`.
 */

void
command_init(command_t *self, const char *name, const char *version) {
  self->arg = NULL;
  self->name = name;
  self->version = version;
  self->option_count = self->argc = 0;
  self->usage = "[options]";
  command_option(self, "-V", "--version", "output program version", command_version);
  command_option(self, "-h", "--help", "output help information", command_help);
}

/*
 * Parse argname from `str`. For example
 * Take "--required <arg>" and populate `flag`
 * with "--required" and `arg` with "<arg>".
 */

static void
parse_argname(const char *str, char *flag, char *arg) {
  int buffer = 0;
  size_t flagpos = 0;
  size_t argpos = 0;
  size_t len = strlen(str);

  for (int i = 0; i < len; ++i) {
    if (buffer || '[' == str[i] || '<' == str[i]) {
      buffer = 1;
      arg[argpos++] = str[i];
    } else {
      if (' ' == str[i]) continue;
      flag[flagpos++] = str[i];
    }
  }

  arg[argpos] = 0;
  flag[flagpos] = 0;
}

/*
 * Define an option.
 */

void
command_option(command_t *self, const char *small, const char *large, const char *desc, command_callback_t cb) {
  int n = self->option_count++;
  if (n == COMMANDER_MAX_OPTIONS) error("Maximum option definitions exceeded");
  command_option_t *option = &self->options[n];
  option->cb = cb;
  option->small = small;
  option->description = desc;
  option->required_arg = option->optional_arg = 0;
  option->large_with_arg = large;
  option->argname = malloc(strlen(large));
  assert(option->argname);
  option->large = malloc(strlen(large));
  assert(option->large);
  parse_argname(large, option->large, option->argname);
  if ('[' == option->argname[0]) option->optional_arg = 1;
  if ('<' == option->argname[0]) option->required_arg = 1;
}

/*
 * Parse `argv`.
 * 
 * TODO: list of short opts (-vo as -v -o)
 */

void
command_parse(command_t *self, int argc, char **argv) {
  int literal = 0;

  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    for (int j = 0; j < self->option_count; ++j) {
      command_option_t *option = &self->options[j];

      // match flag
      if (!strcmp(arg, option->small) || !strcmp(arg, option->large)) {
        self->arg = NULL;

        // required
        if (option->required_arg) {
          arg = argv[++i];
          if (!arg || '-' == arg[0]) {
            fprintf(stderr, "%s %s argument required\n", option->large, option->argname);
            exit(1);
          }
          self->arg = arg;
        }

        // optional
        if (option->optional_arg) {
          if (argv[i + 1] && '-' != argv[i + 1][0]) {
            self->arg = argv[++i];
          }
        }

        // invoke callback
        option->cb(self);
        goto match;
      }
    }

    // --
    if ('-' == arg[0] && '-' == arg[1] && 0 == arg[2]) {
      literal = 1;
      goto match;
    }

    // unrecognized
    if ('-' == arg[0] && !literal) {
      fprintf(stderr, "unrecognized flag %s\n", arg);
      exit(1);
    }

    int n = self->argc++;
    if (n == COMMANDER_MAX_ARGS) error("Maximum number of arguments exceeded");
    self->argv[n] = (char *) arg;
    match:;
  }
}