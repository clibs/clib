// fuzz_cache.c — libFuzzer harness for the clib package cache
// (src/common/clib-cache.c).
//
// Threat model (see coverage-pipeline-report.md in the OSS-Fuzz repo):
//   * Untrusted input: the author/name/version strings that reach the cache
//     API in production derive from dependency spec strings
//     ("owner/name@version") inside third-party clib.json/package.json
//     manifests processed by `clib install` / `clib update`.
//   * Trust boundary: the clib_cache_* entry points.
//   * The attacker controls the full string content (unbounded length, any
//     bytes except the manifest structural ones).
//   * Expected bug classes: json_cache_path() / package_cache_path()
//     sprintf() unbounded author/name/version into fixed BUFSIZ stack
//     buffers (stack-buffer-overflow); filesystem path handling around
//     fs_write/unlink/copy_dir/rimraf; cache expiry logic.
//
// Harness shape (component API / stateful session):
// LLVMFuzzerInitialize redirects HOME to a private scratch directory,
// initialises the cache, creates a fixed two-file "package source" directory
// plus a load target directory, and pre-seeds one json cache entry and the
// search cache so the has/read "hit" branches are reachable. Each iteration
// derives three '\n'-separated strings (author, name, version) from the
// input and applies one cache operation selected by the first byte.
//
// Notes:
//   * The package save/load/delete operations receive path-sanitised copies
//     (every '/' replaced by '_'): an unsanitised path would let the fuzzer
//     steer rimraf()/copy_dir() at arbitrary directories outside the cache
//     (including this fuzzing run's own bind-mounted /out), destroying the
//     environment. The traversal risk itself is recorded as a candidate
//     finding in the pipeline report and must be reproduced in a controlled
//     setting, not inside the fuzzing loop.
//   * Write operations clean up after themselves within the same iteration
//     so persistent state cannot grow without bound across a long session.
//   * detect_leaks=0 in fuzz_cache.options is the pre-existing suppression
//     carried over unchanged from the existing fuzz_manifest integration.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../../src/common/clib-cache.h"

static char scratch_dir[] = "/tmp/clib-fuzz-cache-XXXXXX";
static char pkg_src[512];
static char pkg_target[512];

static char *dup_bytes(const uint8_t *p, size_t n) {
  char *s = (char *)malloc(n + 1);
  if (!s)
    return NULL;
  memcpy(s, p, n);
  s[n] = '\0';
  return s;
}

// Copy of `s` with every '/' replaced by '_' (path sanitisation, see the
// file header comment for the rationale).
static char *sanitize_path_parts(const char *s) {
  char *out = strdup(s);
  if (!out)
    return NULL;
  for (char *p = out; *p; p++) {
    if ('/' == *p)
      *p = '_';
  }
  return out;
}

static void write_file(const char *path, const char *content) {
  FILE *fp = fopen(path, "w");
  if (!fp)
    return;
  fputs(content, fp);
  fclose(fp);
}

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc;
  (void)argv;

  if (!mkdtemp(scratch_dir)) {
    fprintf(stderr, "fuzz_cache: mkdtemp failed\n");
    return 1;
  }
  setenv("HOME", scratch_dir, 1);

  if (-1 == clib_cache_init(3600)) {
    fprintf(stderr, "fuzz_cache: clib_cache_init failed\n");
    return 1;
  }

  snprintf(pkg_src, sizeof(pkg_src), "%s/pkg-src", scratch_dir);
  snprintf(pkg_target, sizeof(pkg_target), "%s/pkg-target", scratch_dir);
  mkdir(pkg_src, 0755);
  mkdir(pkg_target, 0755);

  char path[600];
  snprintf(path, sizeof(path), "%s/a.c", pkg_src);
  write_file(path, "int clib_fuzz_a;\n");
  snprintf(path, sizeof(path), "%s/a.h", pkg_src);
  write_file(path, "int clib_fuzz_a;\n");

  // Pre-seed a json cache entry and a search cache entry so the has/read
  // "hit" branches are reachable; corpus seeds use the same triple.
  clib_cache_save_json((char *)"clibs", (char *)"seed", (char *)"1.0.0",
                       (char *)"{\"name\":\"seed\"}");
  clib_cache_save_search((char *)"cached search payload");
  return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < 4)
    return 0;

  const unsigned op = data[0] % 16;

  // Input layout: <op byte> author \n name \n version
  const uint8_t *p = data + 1;
  size_t rem = size - 1;
  char *author = NULL, *name = NULL, *version = NULL;
  char *s_author = NULL, *s_name = NULL, *s_version = NULL;

  const uint8_t *nl = (const uint8_t *)memchr(p, '\n', rem);
  if (nl) {
    author = dup_bytes(p, nl - p);
    rem -= (nl - p) + 1;
    p = nl + 1;
    nl = (const uint8_t *)memchr(p, '\n', rem);
    if (nl) {
      name = dup_bytes(p, nl - p);
      rem -= (nl - p) + 1;
      version = dup_bytes(nl + 1, rem);
    } else {
      name = dup_bytes(p, rem);
      version = dup_bytes((const uint8_t *)"", 0);
    }
  } else {
    author = dup_bytes(p, rem);
    name = dup_bytes((const uint8_t *)"", 0);
    version = dup_bytes((const uint8_t *)"", 0);
  }

  if (author && name && version) {
    s_author = sanitize_path_parts(author);
    s_name = sanitize_path_parts(name);
    s_version = sanitize_path_parts(version);
  }

  if (author && name && version && s_author && s_name && s_version) {
    switch (op) {
    case 0:
      // expiry matrix: re-init with a fuzzed expiration (0 => everything
      // counts as expired; large => nothing expires during a run)
      (void)clib_cache_init((time_t)((data[1] << 8) | data[2]));
      break;
    case 1:
      (void)clib_cache_has_json(author, name, version);
      break;
    case 2: {
      char *json = clib_cache_read_json(author, name, version);
      free(json);
      break;
    }
    case 3:
      (void)clib_cache_save_json(author, name, version, name);
      (void)clib_cache_delete_json(author, name, version);
      break;
    case 4:
      (void)clib_cache_has_search();
      break;
    case 5: {
      char *search = clib_cache_read_search();
      free(search);
      break;
    }
    case 6:
      (void)clib_cache_save_search(name);
      break;
    case 7:
      (void)clib_cache_delete_search();
      break;
    case 8:
      (void)clib_cache_has_package(author, name, version);
      break;
    case 9:
      (void)clib_cache_is_expired_package(author, name, version);
      break;
    case 10:
      if (0 == clib_cache_save_package(s_author, s_name, s_version, pkg_src)) {
        (void)clib_cache_load_package(s_author, s_name, s_version, pkg_target);
        (void)clib_cache_delete_package(s_author, s_name, s_version);
      }
      break;
    case 11:
      (void)clib_cache_load_package(s_author, s_name, s_version, pkg_target);
      break;
    case 12:
      (void)clib_cache_delete_package(s_author, s_name, s_version);
      break;
    case 13:
      (void)clib_cache_dir();
      break;
    case 14:
      (void)clib_cache_meta_dir();
      break;
    case 15:
      (void)clib_cache_meta_init();
      break;
    default:
      break;
    }
  }

  free(s_author);
  free(s_name);
  free(s_version);
  free(author);
  free(name);
  free(version);
  return 0;
}

