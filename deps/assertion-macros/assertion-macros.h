
//
// assertion-macros.h
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//


#ifndef ASSERTION_MACROS_H
#define ASSERTION_MACROS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int __assert_bail = 0;
static int __assert_failures = 0;

/*
 * Bail at first failing assertion
 */

#define assert_bail() __assert_bail = !__assert_bail;

/*
 * Get the number of failed assertions
 */

#define assert_failures() __assert_failures

/*
 * Reset the number of failed assertions
 */

#define assert_reset() do {                                     \
  __assert_failures = 0;                                        \
} while(0);

// don't clobber assert
#ifndef assert
#  define assert assert_ok
#endif

/*
 * Assert that `expr` evaluates to something truthy
 */

#define assert_ok(expr) do {                                    \
  if (!(expr)) {                                                \
    __assert_failures++;                                        \
    fprintf(                                                    \
        stderr                                                  \
      , "Assertion error: %s (%s:%d)\n"                         \
      , #expr                                                   \
      , __FILE__                                                \
      , __LINE__                                                \
    );                                                          \
    if (__assert_bail) abort();                                 \
  }                                                             \
} while(0);

/*
 * Assert that `expr` is NULL
 */

#define assert_null(expr) do {                                  \
  if ((expr) != NULL) {                                         \
    __assert_failures++;                                        \
    fprintf(                                                    \
        stderr                                                  \
      , "Assertion error: %s is NULL (%s:%d)\n"                 \
      , #expr                                                   \
      , __FILE__                                                \
      , __LINE__                                                \
    );                                                          \
    if (__assert_bail) abort();                                 \
  }                                                             \
} while(0);

/*
 * Assert that `expr` is not NULL
 */

#define assert_not_null(expr) do {                              \
  if ((expr) == NULL) {                                         \
    __assert_failures++;                                        \
    fprintf(                                                    \
        stderr                                                  \
      , "Assertion error: %s is not NULL (%s:%d)\n"             \
      , #expr                                                   \
      , __FILE__                                                \
      , __LINE__                                                \
    );                                                          \
    if (__assert_bail) abort();                                 \
  }                                                             \
} while(0);

/*
 * Assert that `a` is equal to `b`
 */

#define assert_equal(a, b) do {                                 \
  if (a != b) {                                                 \
    __assert_failures++;                                        \
    fprintf(                                                    \
        stderr                                                  \
      , "Assertion error: %d == %d (%s:%d)\n"                   \
      , a                                                       \
      , b                                                       \
      , __FILE__                                                \
      , __LINE__                                                \
    );                                                          \
    if (__assert_bail) abort();                                 \
  }                                                             \
} while(0);

/*
 * Assert that `a` is not equal to `b`
 */

#define assert_not_equal(a, b) do {                             \
  if (a == b) {                                                 \
    __assert_failures++;                                        \
    fprintf(                                                    \
        stderr                                                  \
      , "Assertion error: %d != %d (%s:%d)\n"                   \
      , a                                                       \
      , b                                                       \
      , __FILE__                                                \
      , __LINE__                                                \
    );                                                          \
    if (__assert_bail) abort();                                 \
  }                                                             \
} while(0);

/*
 * Assert that `a` is equal to `b`
 */

#define assert_str_equal(a, b) do {                             \
  if (0 != strcmp(a, b))  {                                     \
    __assert_failures++;                                        \
    fprintf(                                                    \
        stderr                                                  \
      , "Assertion error: \"%s\" == \"%s\" (%s:%d)\n"           \
      , a                                                       \
      , b                                                       \
      , __FILE__                                                \
      , __LINE__                                                \
    );                                                          \
    if (__assert_bail) abort();                                 \
  }                                                             \
} while(0);

/*
 * Assert that `a` is not equal to `b`
 */

#define assert_str_not_equal(a, b) do {                         \
  if (0 == strcmp(a, b))  {                                     \
    __assert_failures++;                                        \
    fprintf(                                                    \
        stderr                                                  \
      , "Assertion error: \"%s\" != \"%s\" (%s:%d)\n"           \
      , a                                                       \
      , b                                                       \
      , __FILE__                                                \
      , __LINE__                                                \
    );                                                          \
    if (__assert_bail) abort();                                 \
  }                                                             \
} while(0);

#endif
