#include "clib-secrets.h"
#include "fs/fs.h"
#include "list/list.h"
#include "logger/logger.h"
#include "parson/parson.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strdup/strdup.h>

struct clib_secret {
  char *hostname;
  char *secret;
};

struct clib_secret_handle {
  list_t *secrets;
};

clib_secrets_t clib_secrets_load_from_file(const char *file) {
  if (-1 == fs_exists(file)) {
    //logger_warn("warning", "Secrets file %s does not exist.", file);
    return NULL;
  }

  //logger_info("info", "Reading secrets from %s.", file);

  char* json = NULL;
  json = fs_read(file);
  if (NULL == json) {
    return NULL;
  }

  JSON_Value *root = json_parse_string(json);
  if (root == NULL) {
    logger_error("error", "unable to parse secrets JSON");
    return NULL;
  }

  JSON_Object *json_object = NULL;
  if (!(json_object = json_value_get_object(root))) {
    logger_error("error", "Invalid json file, root is not an object.");
    return NULL;
  }

  clib_secrets_t handle = malloc(sizeof(struct clib_secret_handle));

  if (!(handle->secrets = list_new())) {
    free(json);
    free(handle);

    return NULL;
  }

  for (unsigned int i = 0; i < json_object_get_count(json_object); i++) {
    const char *domain = json_object_get_name(json_object, i);
    const char *secret = json_object_get_string(json_object, domain);

    struct clib_secret *secret_struct = malloc(sizeof(struct clib_secret));
    secret_struct->hostname = strdup(domain);
    secret_struct->secret = strdup(secret);

    list_rpush(handle->secrets, list_node_new(secret_struct));
  }

  return handle;
}

char *clib_secret_find_for_hostname(clib_secrets_t secrets, const char *hostname) {
  if (secrets == NULL) {
    return NULL;
  }
  list_iterator_t *iterator = list_iterator_new(secrets->secrets, LIST_HEAD);
  list_node_t *node;
  while ((node = list_iterator_next(iterator))) {
    struct clib_secret *secret = node->val;
    if (strcmp(hostname, secret->hostname) == 0) {
      list_iterator_destroy(iterator);
      return secret->secret;
    }
  }

  list_iterator_destroy(iterator);
  return NULL;
}
