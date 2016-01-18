
//
// clib-package.c
//
// Copyright (c) 2014 Stephen Mathieson
// MIT license
//

#include <stdlib.h>
#include <libgen.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "strdup/strdup.h"
#include "parson/parson.h"
#include "substr/substr.h"
#include "http-get/http-get.h"
#include "mkdirp/mkdirp.h"
#include "fs/fs.h"
#include "path-join/path-join.h"
#include "logger/logger.h"
#include "parse-repo/parse-repo.h"
#include "debug/debug.h"

#include "clib-package.h"

#ifndef DEFAULT_REPO_VERSION
#define DEFAULT_REPO_VERSION "master"
#endif

#ifndef DEFAULT_REPO_OWNER
#define DEFAULT_REPO_OWNER "clibs"
#endif

#define GITHUB_CONTENT_URL "https://raw.githubusercontent.com/"

debug_t _debugger;

#define _debug(...) ({                                         \
  if (!(_debugger.name)) debug_init(&_debugger, "clib-package"); \
  debug(&_debugger, __VA_ARGS__);                               \
})

/**
 * Pre-declare prototypes.
 */

static inline char *
json_object_get_string_safe(JSON_Object *, const char *);

static inline char *
json_array_get_string_safe(JSON_Array *, int);

static inline char *
clib_package_file_url(const char *, const char *);

static inline char *
clib_package_slug(const char *, const char *, const char *);

static inline char *
clib_package_repo(const char *, const char *);

static inline list_t *
parse_package_deps(JSON_Object *);

static inline int
install_packages(list_t *, const char *, int);


/**
 * Create a copy of the result of a `json_object_get_string`
 * invocation.  This allows us to `json_value_free()` the
 * parent `JSON_Value` without destroying the string.
 */

static inline char *
json_object_get_string_safe(JSON_Object *obj, const char *key) {
  const char *val = json_object_get_string(obj, key);
  if (!val) return NULL;
  return strdup(val);
}

/**
 * Create a copy of the result of a `json_array_get_string`
 * invocation.  This allows us to `json_value_free()` the
 * parent `JSON_Value` without destroying the string.
 */

static inline char *
json_array_get_string_safe(JSON_Array *array, int index) {
  const char *val = json_array_get_string(array, index);
  if (!val) return NULL;
  return strdup(val);
}

/**
 * Build a URL for `file` of the package belonging to `url`
 */

static inline char *
clib_package_file_url(const char *url, const char *file) {
  if (!url || !file) return NULL;

  int size =
      strlen(url)
    + 1  // /
    + strlen(file)
    + 1  // \0
    ;

  char *res = malloc(size);
  if (res) {
    memset(res, '\0', size);
    sprintf(res, "%s/%s", url, file);
  }
  return res;
}

/**
 * Build a slug
 */

static inline char *
clib_package_slug(const char *author, const char *name, const char *version) {
  int size =
      strlen(author)
    + 1 // /
    + strlen(name)
    + 1 // @
    + strlen(version)
    + 1 // \0
    ;

  char *slug = malloc(size);
  if (slug) {
    memset(slug, '\0', size);
    sprintf(slug, "%s/%s@%s", author, name, version);
  }
  return slug;
}

/**
 * Build a repo
 */

static inline char *
clib_package_repo(const char *author, const char *name) {
  int size =
      strlen(author)
    + 1 // /
    + strlen(name)
    + 1 // \0
    ;

  char *repo = malloc(size);
  if (repo) {
    memset(repo, '\0', size);
    sprintf(repo, "%s/%s", author, name);
  }
  return repo;
}

/**
 * Parse the dependencies in the given `obj` into a `list_t`
 */

static inline list_t *
parse_package_deps(JSON_Object *obj) {
  list_t *list = NULL;

  if (!obj) goto done;
  if (!(list = list_new())) goto done;
  list->free = clib_package_dependency_free;

  for (unsigned int i = 0; i < json_object_get_count(obj); i++) {
    const char *name = NULL;
    char *version = NULL;
    clib_package_dependency_t *dep = NULL;
    int error = 1;

    if (!(name = json_object_get_name(obj, i))) goto loop_cleanup;
    if (!(version = json_object_get_string_safe(obj, name))) goto loop_cleanup;
    if (!(dep = clib_package_dependency_new(name, version))) goto loop_cleanup;
    if (!(list_rpush(list, list_node_new(dep)))) goto loop_cleanup;

    error = 0;

  loop_cleanup:
    if (version) free(version);
    if (error) {
      list_destroy(list);
      list = NULL;
      break;
    }
  }

done:
  return list;
}

static inline int
install_packages(list_t *list, const char *dir, int verbose) {
  list_node_t *node = NULL;
  list_iterator_t *iterator = NULL;
  int rc = -1;

  if (!list || !dir) goto cleanup;

  iterator = list_iterator_new(list, LIST_HEAD);
  if (NULL == iterator) goto cleanup;

  while ((node = list_iterator_next(iterator))) {
    clib_package_dependency_t *dep = NULL;
    char *slug = NULL;
    clib_package_t *pkg = NULL;
    int error = 1;

    dep = (clib_package_dependency_t *) node->val;
    slug = clib_package_slug(dep->author, dep->name, dep->version);
    if (NULL == slug) goto loop_cleanup;

    pkg = clib_package_new_from_slug(slug, verbose);
    if (NULL == pkg) goto loop_cleanup;

    if (-1 == clib_package_install(pkg, dir, verbose)) goto loop_cleanup;

    error = 0;

  loop_cleanup:
    if (slug) free(slug);
    if (pkg) clib_package_free(pkg);
    if (error) {
      list_iterator_destroy(iterator);
      iterator = NULL;
      rc = -1;
      goto cleanup;
    }
  }

  rc = 0;

cleanup:
  if (iterator) list_iterator_destroy(iterator);
  return rc;
}

/**
 * Create a new clib package from the given `json`
 */

clib_package_t *
clib_package_new(const char *json, int verbose) {
  clib_package_t *pkg = NULL;
  JSON_Value *root = NULL;
  JSON_Object *json_object = NULL;
  JSON_Array *src = NULL;
  JSON_Object *deps = NULL;
  JSON_Object *devs = NULL;
  int error = 1;

  if (!json) goto cleanup;
  if (!(root = json_parse_string(json))) {
    logger_error("error", "unable to parse json");
    goto cleanup;
  }
  if (!(json_object = json_value_get_object(root))) {
    logger_error("error", "invalid package.json");
    goto cleanup;
  }
  if (!(pkg = malloc(sizeof(clib_package_t)))) goto cleanup;

  memset(pkg, '\0', sizeof(clib_package_t));

  pkg->json = strdup(json);
  pkg->name = json_object_get_string_safe(json_object, "name");
  pkg->repo = json_object_get_string_safe(json_object, "repo");
  pkg->version = json_object_get_string_safe(json_object, "version");
  pkg->license = json_object_get_string_safe(json_object, "license");
  pkg->description = json_object_get_string_safe(json_object, "description");
  pkg->install = json_object_get_string_safe(json_object, "install");
  pkg->makefile = json_object_get_string_safe(json_object, "makefile");

  _debug("creating package: %s", pkg->repo);

  // TODO npm-style "repository" (thlorenz/gumbo-parser.c#1)
  if (pkg->repo) {
    pkg->author = parse_repo_owner(pkg->repo, DEFAULT_REPO_OWNER);
    // repo name may not be package name (thing.c -> thing)
    pkg->repo_name = parse_repo_name(pkg->repo);
  } else {
    if (verbose) logger_warn("warning", "missing repo in package.json");
    pkg->author = NULL;
    pkg->repo_name = NULL;
  }

  if ((src = json_object_get_array(json_object, "src"))) {
    if (!(pkg->src = list_new())) goto cleanup;
    pkg->src->free = free;
    for (unsigned int i = 0; i < json_array_get_count(src); i++) {
      char *file = json_array_get_string_safe(src, i);
      _debug("file: %s", file);
      if (!file) goto cleanup;
      if (!(list_rpush(pkg->src, list_node_new(file)))) goto cleanup;
    }
  } else {
    _debug("no src files listed in package.json");
    pkg->src = NULL;
  }

  if ((deps = json_object_get_object(json_object, "dependencies"))) {
    if (!(pkg->dependencies = parse_package_deps(deps))) {
      goto cleanup;
    }
  } else {
    _debug("no dependencies listed in package.json");
    pkg->dependencies = NULL;
  }

  if ((devs = json_object_get_object(json_object, "development"))) {
    if (!(pkg->development = parse_package_deps(devs))) {
      goto cleanup;
    }
  } else {
    _debug("no development dependencies listed in package.json");
    pkg->development = NULL;
  }

  error = 0;

cleanup:
  if (root) json_value_free(root);
  if (error && pkg) {
    clib_package_free(pkg);
    pkg = NULL;
  }
  return pkg;
}

/**
 * Create a package from the given repo `slug`
 */

clib_package_t *
clib_package_new_from_slug(const char *slug, int verbose) {
  char *author = NULL;
  char *name = NULL;
  char *version = NULL;
  char *url = NULL;
  char *json_url = NULL;
  char *repo = NULL;
  http_get_response_t *res = NULL;
  clib_package_t *pkg = NULL;

  // parse chunks
  if (!slug) goto error;
  _debug("creating package: %s", slug);
  if (!(author = parse_repo_owner(slug, DEFAULT_REPO_OWNER))) goto error;
  if (!(name = parse_repo_name(slug))) goto error;
  if (!(version = parse_repo_version(slug, DEFAULT_REPO_VERSION))) goto error;
  if (!(url = clib_package_url(author, name, version))) goto error;
  if (!(json_url = clib_package_file_url(url, "package.json"))) goto error;

  _debug("author: %s", author);
  _debug("name: %s", name);
  _debug("version: %s", version);

  // fetch json
  if (verbose) logger_info("fetch", "%s/%s:package.json", author, name);
  _debug("GET %s", json_url);
  res = http_get(json_url);
  _debug("status: %d", res->status);
  if (!res || !res->ok) {
    logger_error("error", "unable to fetch %s/%s:package.json", author, name);
    goto error;
  }

  free(json_url);
  json_url = NULL;
  free(name);
  name = NULL;

  // build package
  pkg = clib_package_new(res->data, verbose);
  http_get_free(res);
  res = NULL;
  if (!pkg) goto error;

  // force version number
  if (pkg->version) {
    if (version) {
      if (0 != strcmp(version, DEFAULT_REPO_VERSION)) {
        _debug("forcing version number: %s (%s)", version, pkg->version);
        free(pkg->version);
        pkg->version = version;
      } else {
        free(version);
      }
    }
  } else {
    pkg->version = version;
  }

  // force package author (don't know how this could fail)
  if (pkg->author) {
    if (0 != strcmp(author, pkg->author)) {
      free(pkg->author);
      pkg->author = author;
    } else {
      free(author);
    }
  } else {
    pkg->author = author;
  }

  if (!(repo = clib_package_repo(pkg->author, pkg->name))) goto error;

  if (pkg->repo) {
    if (0 != strcmp(repo, pkg->repo)) {
      free(url);
      if (!(url = clib_package_url_from_repo(pkg->repo, pkg->version)))
        goto error;
    }
    free(repo);
    repo = NULL;
  } else {
    pkg->repo = repo;
  }

  pkg->url = url;
  return pkg;

error:
  free(author);
  free(name);
  free(version);
  free(url);
  free(json_url);
  free(repo);
  http_get_free(res);
  if (pkg) clib_package_free(pkg);
  return NULL;
}

/**
 * Get a slug for the package `author/name@version`
 */

char *
clib_package_url(const char *author, const char *name, const char *version) {
  if (!author || !name || !version) return NULL;
  int size =
      strlen(GITHUB_CONTENT_URL)
    + strlen(author)
    + 1 // /
    + strlen(name)
    + 1 // /
    + strlen(version)
    + 1 // \0
    ;

  char *slug = malloc(size);
  if (slug) {
    memset(slug, '\0', size);
    sprintf(slug
      , GITHUB_CONTENT_URL "%s/%s/%s"
      , author
      , name
      , version);
  }
  return slug;
}

char *
clib_package_url_from_repo(const char *repo, const char *version) {
  if (!repo || !version) return NULL;
  int size =
      strlen(GITHUB_CONTENT_URL)
    + strlen(repo)
    + 1 // /
    + strlen(version)
    + 1 // \0
    ;

  char *slug = malloc(size);
  if (slug) {
    memset(slug, '\0', size);
    sprintf(slug
      , GITHUB_CONTENT_URL "%s/%s"
      , repo
      , version);
  }
  return slug;
}

/**
 * Parse the package author from the given `slug`
 */

char *
clib_package_parse_author(const char *slug) {
  return parse_repo_owner(slug, DEFAULT_REPO_OWNER);
}

/**
 * Parse the package version from the given `slug`
 */

char *
clib_package_parse_version(const char *slug) {
  return parse_repo_version(slug, DEFAULT_REPO_VERSION);
}

/**
 * Parse the package name from the given `slug`
 */

char *
clib_package_parse_name(const char *slug) {
  return parse_repo_name(slug);
}

/**
 * Create a new package dependency from the given `repo` and `version`
 */

clib_package_dependency_t *
clib_package_dependency_new(const char *repo, const char *version) {
  if (!repo || !version) return NULL;

  clib_package_dependency_t *dep = malloc(sizeof(clib_package_dependency_t));
  if (!dep) {
    return NULL;
  }

  dep->version = 0 == strcmp("*", version)
    ? strdup(DEFAULT_REPO_VERSION)
    : strdup(version);
  dep->name = clib_package_parse_name(repo);
  dep->author = clib_package_parse_author(repo);

  _debug("dependency: %s/%s@%s", dep->author, dep->name, dep->version);
  return dep;
}

/**
 * Fetch a file associated with the given `pkg`.
 *
 * Returns 0 on success.
 */

static int
fetch_package_file(
      clib_package_t *pkg
    , const char *dir
    , char *file
    , int verbose
  ) {
  char *url = NULL;
  char *path = NULL;
  int rc = 0;

  _debug("fetch file: %s/%s", pkg->repo, file);

  if (!(url = clib_package_file_url(pkg->url, file))) {
    return 1;
  }

  _debug("file URL: %s", url);

  if (!(path = path_join(dir, basename(file)))) {
    rc = 1;
    goto cleanup;
  }

  if (verbose) logger_info("fetch", "%s:%s", pkg->repo, file);

  if (-1 == http_get_file(url, path)) {
    logger_error("error", "unable to fetch %s:%s", pkg->repo, file);
    rc = 1;
    goto cleanup;
  }

  if (verbose) logger_info("save", path);

cleanup:
  free(url);
  free(path);
  return rc;
}

/**
 * Install the given `pkg` in `dir`
 */

int
clib_package_install(clib_package_t *pkg, const char *dir, int verbose) {
  char *pkg_dir = NULL;
  char *package_json = NULL;
  list_iterator_t *iterator = NULL;
  int rc = -1;

  if (!pkg || !dir) goto cleanup;
  if (!(pkg_dir = path_join(dir, pkg->name))) goto cleanup;

  _debug("mkdir -p %s", pkg_dir);
  // create directory for pkg
  if (-1 == mkdirp(pkg_dir, 0777)) goto cleanup;

  if (NULL == pkg->url) {
    pkg->url = clib_package_url(pkg->author
      , pkg->repo_name
      , pkg->version);
    if (NULL == pkg->url) goto cleanup;
  }

  // write package.json
  if (!(package_json = path_join(pkg_dir, "package.json"))) goto cleanup;
  _debug("write: %s", package_json);
  if (-1 == fs_write(package_json, pkg->json)) {
    logger_error("error", "Failed to write %s", package_json);
    goto cleanup;
  }

  // fetch makefile
  if (pkg->makefile) {
    _debug("fetch: %s/%s", pkg->repo, pkg->makefile);
    if (0 != fetch_package_file(pkg, pkg_dir, pkg->makefile, verbose)) {
      goto cleanup;
    }
  }

  // if no sources are listed, just install
  if (NULL == pkg->src) goto install;

  iterator = list_iterator_new(pkg->src, LIST_HEAD);
  list_node_t *source;
  while ((source = list_iterator_next(iterator))) {
    if (0 != fetch_package_file(pkg, pkg_dir, source->val, verbose)) {
      list_iterator_destroy(iterator);
      iterator = NULL;
      rc = -1;
      goto cleanup;
    }
  }

install:
  rc = clib_package_install_dependencies(pkg, dir, verbose);

cleanup:
  if (pkg_dir) free(pkg_dir);
  if (package_json) free(package_json);
  if (iterator) list_iterator_destroy(iterator);
  return rc;
}

/**
 * Install the given `pkg`'s dependencies in `dir`
 */

int
clib_package_install_dependencies(clib_package_t *pkg
    , const char *dir
    , int verbose) {
  if (!pkg || !dir) return -1;
  if (NULL == pkg->dependencies) return 0;

  return install_packages(pkg->dependencies, dir, verbose);
}

/**
 * Install the given `pkg`'s development dependencies in `dir`
 */

int
clib_package_install_development(clib_package_t *pkg
    , const char *dir
    , int verbose) {
  if (!pkg || !dir) return -1;
  if (NULL == pkg->development) return 0;

  return install_packages(pkg->development, dir, verbose);
}

/**
 * Free a clib package
 */

void
clib_package_free(clib_package_t *pkg) {
  free(pkg->author);
  free(pkg->description);
  free(pkg->install);
  free(pkg->json);
  free(pkg->license);
  free(pkg->name);
  free(pkg->makefile);
  free(pkg->repo);
  free(pkg->repo_name);
  free(pkg->url);
  free(pkg->version);
  if (pkg->src) list_destroy(pkg->src);
  if (pkg->dependencies) list_destroy(pkg->dependencies);
  if (pkg->development) list_destroy(pkg->development);
  free(pkg);
}

void
clib_package_dependency_free(void *_dep) {
  clib_package_dependency_t *dep = (clib_package_dependency_t *) _dep;
  free(dep->name);
  free(dep->author);
  free(dep->version);
  free(dep);
}
