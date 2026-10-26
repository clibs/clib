// fuzz_slug.c — libFuzzer harness for the slug / version / URL / dependency
// / options surface of src/common/clib-package.c.
//
// Threat model (see coverage-pipeline-report.md in the OSS-Fuzz repo):
//   * Untrusted input: package slug strings ("owner/name@version") derive
//     from dependency spec strings inside third-party clib.json and
//     package.json manifests, and from the command line.
//   * Trust boundary: clib_package_parse_author / parse_name /
//     parse_version, clib_package_url, clib_package_url_from_repo,
//     clib_package_dependency_new and clib_package_set_opts.
//   * The attacker controls the full string content and length.
//   * Expected bug classes: memory safety in the string parsing helpers
//     (parse-repo/substr/strdup paths), URL construction errors feeding the
//     fetcher (wrong URL shapes), and options handling (prefix/token).
//
// Harness shape (component API / option matrix): the first byte is an
// options matrix (skip_cache / force / global / prefix / token bits); the
// rest of the input is split into "slug \n version". Each iteration parses
// the slug through the three parse helpers, installs a derived options
// matrix, builds both URL forms (exercising the token branches), and builds
// and frees a dependency from the same strings.
//
// The static helpers clib_package_slug / clib_package_repo /
// clib_package_file_url are only reachable through the network-bound
// install paths (excluded from fuzzing; see the report) and remain dark.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/common/clib-package.h"

// Storage for the pointers handed to clib_package_set_opts(): the global
// options object keeps the pointers, so they must outlive the iteration.
static char prefix_buf[64];
static char token_buf[64];

static char *dup_bytes(const uint8_t *p, size_t n) {
  char *s = (char *)malloc(n + 1);
  if (!s)
    return NULL;
  memcpy(s, p, n);
  s[n] = '\0';
  return s;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < 4)
    return 0;

  const unsigned flags = data[0];

  // Input layout: <flags byte> slug \n version
  const uint8_t *p = data + 1;
  size_t rem = size - 1;
  char *slug = NULL, *version = NULL;

  const uint8_t *nl = (const uint8_t *)memchr(p, '\n', rem);
  if (nl) {
    slug = dup_bytes(p, nl - p);
    version = dup_bytes(nl + 1, rem - (nl - p) - 1);
  } else {
    slug = dup_bytes(p, rem);
    version = dup_bytes((const uint8_t *)"", 0);
  }

  if (!slug || !version)
    goto done;

  // 1. slug parsing triple (attacker-shaped slug strings)
  char *author = clib_package_parse_author(slug);
  char *name = clib_package_parse_name(slug);
  char *parsed_version = clib_package_parse_version(slug);

  // 2. options matrix (installed before URL building so the token/prefix
  //    branches inside the URL builders activate); an empty derived string
  //    lights the strlen()==0 reset branches inside clib_package_set_opts
  snprintf(prefix_buf, sizeof(prefix_buf), "%s", name ? name : "prefix");
  snprintf(token_buf, sizeof(token_buf), "%s", author ? author : "token");

  clib_package_opts_t opts;
  memset(&opts, 0, sizeof(opts));
  opts.skip_cache = flags & 0x01;
  opts.force = (flags >> 1) & 0x01;
  opts.global = (flags >> 2) & 0x01;
  opts.prefix = (flags & 0x08) ? prefix_buf : NULL;
  opts.token = (flags & 0x10) ? token_buf : NULL;
  opts.concurrency = (flags >> 5) & 0x07;
  clib_package_set_opts(opts);

  // 3. URL builders (exact-size allocation; token-aware branches)
  {
    const char *a = author ? author : "clibs";
    const char *n = name ? name : "clib";
    const char *v = parsed_version ? parsed_version : "master";
    char *url = clib_package_url(a, n, v);
    char *url_from_repo = clib_package_url_from_repo(slug, v);
    free(url);
    free(url_from_repo);
  }

  // 4. dependency construction from the same attacker-shaped strings
  {
    clib_package_dependency_t *dep = clib_package_dependency_new(slug, version);
    if (dep) {
      clib_package_dependency_free(dep);
    }
  }

  free(author);
  free(name);
  free(parsed_version);

done:
  free(slug);
  free(version);
  return 0;
}
