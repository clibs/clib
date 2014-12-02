
#include <stdlib.h>
#include "wildcardcmp.h"

int
wildcardcmp(const char *pattern, const char *string) {
  const char *w = NULL; // last `*`
  const char *s = NULL; // last checked char

  // malformed
  if (!pattern || !string) return 0;

  // loop 1 char at a time
  while (1) {
    if (!*string) {
      if (!*pattern) return 1;
      if ('*' == *pattern) {
        pattern++;
        continue;
      }
      if (!*s) return 0;
      string = s++;
      pattern = w;
      continue;
    } else {
      if (*pattern != *string) {
        if ('*' == *pattern) {
          w = ++pattern;
          s = string;
          // "*" -> "foobar"
          if (*pattern) continue;
          return 1;
        } else if (w) {
          string++;
          // "*ooba*" -> "foobar"
          continue;
        }
        return 0;
      }
    }

    string++;
    pattern++;
  }

  return 1;
}
