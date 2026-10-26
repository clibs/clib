//
// fuzz_package.c
//
// Copyright (c) 2026 clib authors
// MIT licensed
//
// Exercises clib's package-string parsers and builders that the manifest
// fuzzer never reaches: clib_package_parse_author/name/version,
// clib_package_dependency_new, clib_package_url and clib_package_url_from_repo.
// All of these consume strings taken from untrusted clib.json/package.json
// manifests (author, name, version, repo, dependency "name@version" specs), so
// they are part of clib's attacker-controlled surface.
//
// Input: NUL-separated fields; a field may be empty.
//  [0] manifest json   -> clib_package_new()
//  [1] dep name        -> clib_package_dependency_new([1], [2])
//  [2] dep version
//  [3] author          -> clib_package_parse_author()
//  [4] name            -> clib_package_parse_name()
//  [5] version         -> clib_package_parse_version()
//  [6] author \        -> clib_package_url([6], [7], [8])
//  [7] name
//  [8] version
//  [9] repo            -> clib_package_url_from_repo([9], [10])
// [10] version
//
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/common/clib-package.h"

#define MAX_FIELDS 11
#define MAX_FIELD_LEN 4096

static char *dup_field(const char *s, size_t n) {
  char *out;
  if (n > MAX_FIELD_LEN)
    n = MAX_FIELD_LEN;
  out = malloc(n + 1);
  if (!out)
    return NULL;
  memcpy(out, s, n);
  out[n] = '\0';
  return out;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  char *fields[MAX_FIELDS] = {0};
  size_t idx = 0, start = 0, i;

  for (i = 0; i < size && idx < MAX_FIELDS; i++) {
    if (data[i] == '\0') {
      fields[idx++] = dup_field((const char *)data + start, i - start);
      start = i + 1;
    }
  }
  if (idx < MAX_FIELDS && start < size)
    fields[idx] = dup_field((const char *)data + start, size - start);

  if (fields[0]) {
    clib_package_t *pkg = clib_package_new(fields[0], 0);
    if (pkg)
      clib_package_free(pkg);
  }

  if (fields[1]) {
    clib_package_dependency_t *dep =
        clib_package_dependency_new(fields[1], fields[2]);
    if (dep)
      clib_package_dependency_free(dep);
  }

  if (fields[3]) {
    char *s = clib_package_parse_author(fields[3]);
    free(s);
  }
  if (fields[4]) {
    char *s = clib_package_parse_name(fields[4]);
    free(s);
  }
  if (fields[5]) {
    char *s = clib_package_parse_version(fields[5]);
    free(s);
  }
  if (fields[6]) {
    char *s = clib_package_url(fields[6], fields[7], fields[8]);
    free(s);
  }
  if (fields[9]) {
    char *s = clib_package_url_from_repo(fields[9], fields[10]);
    free(s);
  }

  for (i = 0; i < MAX_FIELDS; i++)
    free(fields[i]);
  return 0;
}
