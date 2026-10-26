// fuzz_validate.c — libFuzzer harness for the clib manifest validator
// (src/common/clib-validate.c).
//
// Threat model (see coverage-pipeline-report.md in the OSS-Fuzz repo):
//   * Untrusted input: clib.json / package.json manifest files. In
//     production the validator runs on manifests fetched from third-party
//     repositories before `clib install` / `clib update` act on them
//     (src/clib-install.c, src/clib-update.c), i.e. exactly the supply
//     chain input an attacker controls.
//   * Trust boundary: clib_validate(file) — bytes-in via a file.
//   * Expected bug classes: memory safety in the JSON parsing (parson) and
//     repo-string parsing (parse-repo) under validation, and validation
//     logic errors.
//
// Harness shape (classic bytes-into-parser): the input is written to a
// fixed file path and validated. The file is overwritten (not unlinked)
// every iteration.
//
// Note: clib-validate.c is not part of the current fuzz build at all (it is
// never referenced by the existing fuzz_manifest link); the OSS-Fuzz side
// build script of this pair compiles it in.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../src/common/clib-validate.h"

#define FUZZ_MANIFEST_PATH "/tmp/clib-fuzz-validate.json"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < 2)
    return 0;

  FILE *fp = fopen(FUZZ_MANIFEST_PATH, "wb");
  if (!fp)
    return 0;
  fwrite(data, 1, size, fp);
  fclose(fp);

  (void)clib_validate(FUZZ_MANIFEST_PATH);
  return 0;
}
