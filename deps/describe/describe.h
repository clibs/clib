
//
// describe.h
//
// Copyright (c) 2013 Stephen Mathieson
// Copyright (c) 2015 Michael Phan-Ba
// MIT licensed
//


#ifndef DESCRIBE_H
#define DESCRIBE_H 1

#include "console-colors/console-colors.h"
#include "assertion-macros/assertion-macros.h"

#define DESCRIBE_VERSION "1.1.0"
#define DESCRIBE_OK      "✓"
#define DESCRIBE_FAIL    "✖"

void __describe_after_spec(const char *specification, int before) {
  if (assert_failures() == before) {
    cc_fprintf(
        CC_FG_DARK_GREEN
      , stdout
      , "    %s"
      , DESCRIBE_OK
    );
  } else {
    cc_fprintf(
        CC_FG_DARK_RED
      , stdout
      , "    %s"
      , DESCRIBE_FAIL
    );
  }
  cc_fprintf(
      CC_FG_GRAY
    , stdout
    , " %s\n"
    , specification
  );
}

/*
 * Describe `suite` with `title`
 */

#define describe(title) for (                     \
  int __run = 0;                                  \
  __run++ == 0 && printf("\n  %s\n", title);      \
  printf("\n")                                    \
)

/*
 * Describe `fn` with `specification`
 */

#define it(specification) for (                   \
  int __before = assert_failures(), __run = 0;    \
  __run++ == 0;                                   \
  __describe_after_spec(specification, __before)  \
)

#endif
