
//
// clib-validate: `clib-validate.c`
//
// Copyright (c) 2021 Stephen Mathieson
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

#define ERROR(err, ...)                                                        \
  ({                                                                           \
    rc = 1;                                                                    \
    logger_error("error", err, __VA_ARGS__);                                   \
    goto done;                                                                 \
  })

#define WARN(warning)                                                          \
  ({                                                                           \
    rc++;                                                                      \
    logger_warn("warning", warning);                                           \
  });

#define WARN_MISSING(key) ({ WARN("missing " #key " in package.json"); })

#define require_string(name)                                                   \
  ({                                                                           \
    const char *__##name = json_object_get_string(obj, #name);                 \
    if (!(__##name))                                                           \
      WARN_MISSING(#name);                                                     \
  })

int clib_validate(const char *file) {
  const char *repo = NULL;
  char *repo_owner = NULL;
  char *repo_name = NULL;
  int rc = 0;
  JSON_Value *root = NULL;
  JSON_Object *obj = NULL;
  JSON_Value *src = NULL;
  JSON_Array *keywords = NULL;

  if (-1 == fs_exists(file))
    ERROR("no such file: %s", file);
  if (!(root = json_parse_file(file)))
    ERROR("malformed file: %s", file);
  if (!(obj = json_value_get_object(root)))
    ERROR("malformed file: %s", file);

  require_string(name);
  require_string(version);
  // TODO: validate semver

  repo = json_object_get_string(obj, "repo");
  if (!repo) {
    WARN_MISSING("repo");
  } else {
    if (!(repo_name = parse_repo_name(repo)))
      WARN("invalid repo");
    if (!(repo_owner = parse_repo_owner(repo, NULL)))
      WARN("invalid repo");
  }

  require_string(description);
  require_string(license);

  src = json_object_get_value(obj, "src");
  if (!src) {
    // if there are no sources, then you need an
    // install key.  otherwise, there's no point
    // in your lib.
    require_string(install);
  } else if (json_value_get_type(src) != JSONArray) {
    WARN("src should be an array")
  }

  if (!(keywords = json_object_get_array(obj, "keywords"))) {
    WARN_MISSING("keywords");
  }

done:
  if (root)
    json_value_free(root);
  return rc;
}
