
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
  command_free(self);
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

  int i;
  for (i = 0; i < self->option_count; ++i) {
    command_option_t *option = &self->options[i];
    printf("    %s, %-25s %s\n"
      , option->small
      , option->large_with_arg
      , option->description);
  }

  printf("\n");
  command_free(self);
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
  self->nargv = NULL;
  command_option(self, "-V", "--version", "output program version", command_version);
  command_option(self, "-h", "--help", "output help information", command_help);
}

/*
 * Free up commander after use.
 */

void
command_free(command_t *self) {
  int i;

  for (i = 0; i < self->option_count; ++i) {
    command_option_t *option = &self->options[i];
    free(option->argname);
    free(option->large);
  }

  if (self->nargv) {
    for (i = 0; self->nargv[i]; ++i) {
      free(self->nargv[i]);
    }
    free(self->nargv);
  }
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
  size_t i;

  for (i = 0; i < len; ++i) {
    if (buffer || '[' == str[i] || '<' == str[i]) {
      buffer = 1;
      arg[argpos++] = str[i];
    } else {
      if (' ' == str[i]) continue;
      flag[flagpos++] = str[i];
    }
  }

  arg[argpos] = '\0';
  flag[flagpos] = '\0';
}

/*
 * Normalize the argument vector by exploding
 * multiple options (if any). For example
 * "foo -abc --scm git" -> "foo -a -b -c --scm git"
 */

static char **
normalize_args(int *argc, char **argv) {
  int size = 0;
  int alloc = *argc + 1;
  char **nargv = malloc(alloc * sizeof(char *));
  int i;

  for (i = 0; argv[i]; ++i) {
    const char *arg = argv[i];
    size_t len = strlen(arg);

    // short flag
    if (len > 2 && '-' == arg[0] && !strchr(arg + 1, '-')) {
      alloc += len - 2;
      nargv = realloc(nargv, alloc * sizeof(char *));
      for (size_t j = 1; j < len; ++j) {
        nargv[size] = malloc(3);
        sprintf(nargv[size], "-%c", arg[j]);
        size++;
      }
      continue;
    }

    // regular arg
    nargv[size] = malloc(len + 1);
    strcpy(nargv[size], arg);
    size++;
  }

  nargv[size] = NULL;
  *argc = size;
  return nargv;
}

/*
 * Define an option.
 */

void
command_option(command_t *self, const char *small, const char *large, const char *desc, command_callback_t cb) {
  if (self->option_count == COMMANDER_MAX_OPTIONS) {
    command_free(self);
    error("Maximum option definitions exceeded");
  }
  int n = self->option_count++;
  command_option_t *option = &self->options[n];
  option->cb = cb;
  option->small = small;
  option->description = desc;
  option->required_arg = option->optional_arg = 0;
  option->large_with_arg = large;
  option->argname = malloc(strlen(large) + 1);
  assert(option->argname);
  option->large = malloc(strlen(large) + 1);
  assert(option->large);
  parse_argname(large, option->large, option->argname);
  if ('[' == option->argname[0]) option->optional_arg = 1;
  if ('<' == option->argname[0]) option->required_arg = 1;
}

/*
 * Parse `argv` (internal).
 * Input arguments should be normalized first
 * see `normalize_args`.
 */

static void
command_parse_args(command_t *self, int argc, char **argv) {
  int literal = 0;
  int i, j;

  for (i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    for (j = 0; j < self->option_count; ++j) {
      command_option_t *option = &self->options[j];

      // match flag
      if (!strcmp(arg, option->small) || !strcmp(arg, option->large)) {
        self->arg = NULL;

        // required
        if (option->required_arg) {
          arg = argv[++i];
          if (!arg || '-' == arg[0]) {
            fprintf(stderr, "%s %s argument required\n", option->large, option->argname);
            command_free(self);
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
      command_free(self);
      exit(1);
    }

    int n = self->argc++;
    if (n == COMMANDER_MAX_ARGS) {
      command_free(self);
      error("Maximum number of arguments exceeded");
    }
    self->argv[n] = (char *) arg;
    match:;
  }
}

/*
 * Parse `argv` (public).
 */

void
command_parse(command_t *self, int argc, char **argv) {
  self->nargv = normalize_args(&argc, argv);
  command_parse_args(self, argc, self->nargv);
  self->argv[self->argc] = NULL;
}
