
//
// commander.h
//
// Copyright (c) 2012 TJ Holowaychuk <tj@vision-media.ca>
//

#ifndef COMMANDER_H
#define COMMANDER_H

/*
 * Max options that can be defined.
 */

#ifndef COMMANDER_MAX_OPTIONS
#define COMMANDER_MAX_OPTIONS 32
#endif

/*
 * Max arguments that can be passed.
 */

#ifndef COMMANDER_MAX_ARGS
#define COMMANDER_MAX_ARGS 32
#endif

/*
 * Command struct.
 */

struct command;

/*
 * Option callback.
 */

typedef void (* command_callback_t)(struct command *self);

/*
 * Command option.
 */

typedef struct {
  int optional_arg;
  int required_arg;
  char *argname;
  char *large;
  const char *small;
  const char *large_with_arg;
  const char *description;
  command_callback_t cb;
} command_option_t;

/*
 * Command.
 */

typedef struct command {
  void *data;
  const char *usage;
  const char *arg;
  const char *name;
  const char *version;
  int option_count;
  command_option_t options[COMMANDER_MAX_OPTIONS];
  int argc;
  char *argv[COMMANDER_MAX_ARGS];
} command_t;

// prototypes

void
command_init(command_t *self, const char *name, const char *version);

void
command_help(command_t *self);

void
command_option(command_t *self, const char *small, const char *large, const char *desc, command_callback_t cb);

void
command_parse(command_t *self, int argc, char **argv);

#endif /* COMMANDER_H */