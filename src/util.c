
//
// util.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#ifdef _WIN32
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

int
vasprintf(char **p, char *fmt, __VALIST argv) {
  int wanted = vsnprintf(*p = NULL, 0, fmt, argv);
  if ((wanted < 0) || ((*p = malloc(1 + wanted)) == NULL)) return -1;
  return vsprintf(*p, fmt, argv);
}

int
asprintf(char **p, char *fmt, ...) {
  va_list argv;
  int r;
  va_start(argv, fmt);
  r = vasprintf(p, fmt, argv);
  va_end(argv);
  return r;
}
#endif
