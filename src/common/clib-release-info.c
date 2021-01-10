//
// clib-release-info.c
//
// Copyright (c) 2016-2021 clib authors
// MIT licensed
//

#include "debug/debug.h"
#include "http-get/http-get.h"
#include "parson/parson.h"
#include "strdup/strdup.h"

#define LATEST_RELEASE_ENDPOINT                                                \
  "https://api.github.com/repos/clibs/clib/releases/latest"

static debug_t debugger;

const char *clib_release_get_latest_tag(void) {
  debug_init(&debugger, "clib-release-info");

  http_get_response_t *res = http_get(LATEST_RELEASE_ENDPOINT);

  JSON_Value *root_json = NULL;
  JSON_Object *json_object = NULL;
  char *tag_name = NULL;

  if (!res->ok) {
    debug(&debugger, "Couldn't lookup latest release");
    goto cleanup;
  }

  if (!(root_json = json_parse_string(res->data))) {
    debug(&debugger, "Unable to parse release JSON response");
    goto cleanup;
  }

  if (!(json_object = json_value_get_object(root_json))) {
    debug(&debugger, "Unable to parse release JSON response object");
    goto cleanup;
  }

  tag_name = strdup(json_object_get_string(json_object, "tag_name"));

  if (!tag_name) {
    debug(&debugger, "strudp(tag_name) failed");
    goto cleanup;
  }

cleanup:
  if (root_json)
    json_value_free(root_json);

  http_get_free(res);

  return tag_name;
}
