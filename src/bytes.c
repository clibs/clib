
//
// bytes.c
//
// Copyright (c) 2012 TJ Holowaychuk <tj@vision-media.ca>
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "bytes.h"

// bytes

#define KB 1024
#define MB 1024 * KB
#define GB 1024 * MB

/*
 * Convert the given `str` to byte count.
 */

long long
string_to_bytes(const char *str) {
  size_t len = strlen(str);
  long long val = strtoll(str, NULL, 10);
  if (!val) return -1;
  if (strstr(str, "kb")) return val * KB;
  if (strstr(str, "mb")) return val * MB;
  if (strstr(str, "gb")) return val * GB;
  return val;
}

/*
 * Convert the given `bytes` to a string. This
 * value must be `free()`d by the user.
 */

char *
bytes_to_string(long long bytes) {
  char *str = malloc(BYTES_MAX);
  if (!str) return NULL;
  long div = 1;
  char *fmt;

  if (bytes < KB) fmt = "%lldb";
  else if (bytes < MB) { fmt = "%lldkb"; div = KB; }
  else if (bytes < GB) { fmt = "%lldmb"; div = MB; }
  else { fmt = "%lldgb"; div = GB; }
  snprintf(str, BYTES_MAX, fmt, bytes / div);

  return str;
}

// tests

#ifdef TEST_BYTES

#include <assert.h>

void
equal(char *a, char *b) {
  if (strcmp(a, b)) {
    printf("expected: %s\n", a);
    printf("actual: %s\n", b);
    exit(1);
  }
}

void
test_string_to_bytes() {
  assert(100 == string_to_bytes("100"));
  assert(100 == string_to_bytes("100b"));
  assert(100 == string_to_bytes("100 bytes"));
  assert(KB == string_to_bytes("1kb"));
  assert(MB == string_to_bytes("1mb"));
  assert(GB == string_to_bytes("1gb"));
  assert(2 * KB == string_to_bytes("2kb"));
  assert(5 * MB == string_to_bytes("5mb"));
}

void
test_bytes_to_string() {
  equal("100b", bytes_to_string(100));
  equal("1kb", bytes_to_string(KB));
  equal("1mb", bytes_to_string(MB));
  equal("5mb", bytes_to_string(MB * 5));
  equal("1gb", bytes_to_string(GB));
  equal("5gb", bytes_to_string((long long) GB * 5));
}

int
main(){
  test_string_to_bytes();
  test_bytes_to_string();
  printf("\n  \e[32m\u2713 \e[90mok\e[0m\n\n");
  return 0;
}

#endif