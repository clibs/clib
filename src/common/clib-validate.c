//
// clib-validate: `clib-validate.c`
//
// Copyright (c) 2014 Stephen Mathieson
// Copyright (c) 2021 clib authors
// MIT licensed
//

#include "fs/fs.h"
#include "logger/logger.h"
#include "parse-repo/parse-repo.h"
#include "parson/parson.h"
#include "path-join/path-join.h"
#include "strdup/strdup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ERROR_FORMAT(err, ...)                                                 \
  ({                                                                           \
    rc = 1;                                                                    \
    logger_error("error", err, __VA_ARGS__);                                   \
    goto done;                                                                 \
  })

#define WARN(warning) ({ logger_warn("warning", warning); });

#define WARN_FORMAT(warning, ...)                                              \
  ({ logger_warn("warning", warning, __VA_ARGS__); });

#define WARN_MISSING(key, file)                                                \
  ({ WARN_FORMAT("missing " #key " in  %s", file); })

#define require_string(name, file)                                             \
  ({                                                                           \
    const char *__##name = json_object_get_string(obj, #name);                 \
    if (!(__##name))                                                           \
      WARN_MISSING(#name, file);                                               \
  })

int clib_validate(const char *file) {
  const char *repo = NULL;
  char *repo_owner = NULL;
  char *repo_name = NULL;
  int rc = 0;
  JSON_Value *root = NULL;
  JSON_Object *obj = NULL;
  JSON_Value *src = NULL;

  if (-1 == fs_exists(file))
    ERROR_FORMAT("no such file: %s", file);
  if (!(root = json_parse_file(file)))
    ERROR_FORMAT("malformed file: %s", file);
  if (!(obj = json_value_get_object(root)))
    ERROR_FORMAT("malformed file: %s", file);

  require_string(name, file);
  require_string(version, file);
  // TODO: validate semver

  repo = json_object_get_string(obj, "repo");
  if (!repo) {
    WARN_MISSING("repo", file);
  } else {
    if (!(repo_name = parse_repo_name(repo)))
      WARN("invalid repo");
    if (!(repo_owner = parse_repo_owner(repo, NULL)))
      WARN("invalid repo");
  }

  require_string(description, file);
  require_string(license, file);

  if (!json_object_get_array(obj, "keywords")) {
    WARN_MISSING("keywords", file);
  }

  src = json_object_get_value(obj, "src");
  if (!src) {

    if (!json_object_get_string(obj, "install"))
      ERROR_FORMAT("Must have either src or install defined in %s", file);

  } else if (json_value_get_type(src) != JSONArray) {
    WARN("src should be an array")
  }

done:
  if (root)
    json_value_free(root);
  return rc;
}
