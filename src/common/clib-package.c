// clib-package.c
//
// Copyright (c) 2014 Stephen Mathieson
// Copyright (c) 2014-2020 clib authors
// MIT license
//

#include "asprintf/asprintf.h"
#include "clib-cache.h"
#include "clib-package.h"
#include "clib-settings.h"
#include "debug/debug.h"
#include "fs/fs.h"
#include "hash/hash.h"
#include "http-get/http-get.h"
#include "logger/logger.h"
#include "mkdirp/mkdirp.h"
#include "parse-repo/parse-repo.h"
#include "parson/parson.h"
#include "strdup/strdup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_PTHREADS
#include <pthread.h>
#include <repository.h>
#endif

#ifndef DEFAULT_REPO_VERSION
#define DEFAULT_REPO_VERSION "master"
#endif

#ifndef DEFAULT_REPO_OWNER
#define DEFAULT_REPO_OWNER "clibs"
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) ||               \
    defined(__MINGW64__)
#define setenv(k, v, _) _putenv_s(k, v)
#define realpath(a, b) _fullpath(a, b, strlen(a))
#endif

static hash_t *visited_packages = 0;

#ifdef HAVE_PTHREADS
typedef struct clib_package_lock clib_package_lock_t;
struct clib_package_lock {
  pthread_mutex_t mutex;
};

static clib_package_lock_t lock = {PTHREAD_MUTEX_INITIALIZER};

#endif

static debug_t _debugger;

#define _debug(...)                                                            \
  ({                                                                           \
    if (!(_debugger.name))                                                     \
      debug_init(&_debugger, "clib-package");                                  \
    debug(&_debugger, __VA_ARGS__);                                            \
  })

#define E_FORMAT(...)                                                          \
  ({                                                                           \
    rc = asprintf(__VA_ARGS__);                                                \
    if (-1 == rc)                                                              \
      goto cleanup;                                                            \
  });

clib_package_opts_t package_opts = {
#ifdef HAVE_PTHREADS
    .concurrency = MAX_THREADS,
#endif
    .skip_cache = 1,
    .prefix = 0,
    .global = 0,
    .force = 0,
    .token = 0,
};

/**
 * Pre-declare prototypes.
 */

static inline char *json_object_get_string_safe(JSON_Object *, const char *);

static inline char *json_array_get_string_safe(JSON_Array *, int);

static inline char *clib_package_file_url(const char *, const char *);


static inline list_t *parse_package_deps(JSON_Object *);

static inline int install_packages(list_t *, const char *, int);

void clib_package_set_opts(clib_package_opts_t o) {
  if (1 == package_opts.skip_cache && 0 == o.skip_cache) {
    package_opts.skip_cache = 0;
  } else if (0 == package_opts.skip_cache && 1 == o.skip_cache) {
    package_opts.skip_cache = 1;
  }

  if (1 == package_opts.global && 0 == o.global) {
    package_opts.global = 0;
  } else if (0 == package_opts.global && 1 == o.global) {
    package_opts.global = 1;
  }

  if (1 == package_opts.force && 0 == o.force) {
    package_opts.force = 0;
  } else if (0 == package_opts.force && 1 == o.force) {
    package_opts.force = 1;
  }

  if (0 != o.prefix) {
    if (0 == strlen(o.prefix)) {
      package_opts.prefix = 0;
    } else {
      package_opts.prefix = o.prefix;
    }
  }

  if (0 != o.token) {
    if (0 == strlen(o.token)) {
      package_opts.token = 0;
    } else {
      package_opts.token = o.token;
    }
  }

  if (o.concurrency) {
    package_opts.concurrency = o.concurrency;
  } else if (o.concurrency < 0) {
    package_opts.concurrency = 0;
  }

  if (package_opts.concurrency < 0) {
    package_opts.concurrency = 0;
  }
}

/**
 * Create a copy of the result of a `json_object_get_string`
 * invocation.  This allows us to `json_value_free()` the
 * parent `JSON_Value` without destroying the string.
 */

static inline char *json_object_get_string_safe(JSON_Object *obj,
                                                const char *key) {
  const char *val = json_object_get_string(obj, key);
  if (!val)
    return NULL;
  return strdup(val);
}

/**
 * Create a copy of the result of a `json_array_get_string`
 * invocation.  This allows us to `json_value_free()` the
 * parent `JSON_Value` without destroying the string.
 */

static inline char *json_array_get_string_safe(JSON_Array *array, int index) {
  const char *val = json_array_get_string(array, index);
  if (!val)
    return NULL;
  return strdup(val);
}

/**
 * Build a URL for `file` of the package belonging to `url`
 */

static inline char *clib_package_file_url(const char *url, const char *file) {
  if (!url || !file)
    return NULL;

  int size = strlen(url) + 1    // /
             + strlen(file) + 1 // \0
      ;

  char *res = malloc(size);
  if (res) {
    memset(res, 0, size);
    sprintf(res, "%s/%s", url, file);
  }
  return res;
}

/**
 * Build a slug
 */

char *clib_package_slug(const char *author, const char *name,
                                      const char *version) {
  int size = strlen(author) + 1    // /
             + strlen(name) + 1    // @
             + strlen(version) + 1 // \0
      ;

  char *slug = malloc(size);
  if (slug) {
    memset(slug, '\0', size);
    sprintf(slug, "%s/%s@%s", author, name, version);
  }
  return slug;
}

/**
 * Load a local package with a manifest.
 */

clib_package_t *clib_package_load_from_manifest(const char *manifest,
                                                int verbose) {
  clib_package_t *pkg = NULL;

  if (-1 == fs_exists(manifest)) {
    logger_error("error", "Missing %s", manifest);
    return NULL;
  }

  logger_info("info", "reading local %s", manifest);

  char *json = fs_read(manifest);
  if (NULL == json)
    goto e1;

  pkg = clib_package_new(json, verbose);

e1:
  free(json);

  return pkg;
}

/**
 * Load a manifest from the current path.
 */

clib_package_t *clib_package_load_local_manifest(int verbose) {
  clib_package_t *pkg = NULL;
  int i = 0;

  do {
    const char *name = NULL;
    name = manifest_names[i];
    pkg = clib_package_load_from_manifest(name, verbose);
  } while (pkg == NULL && NULL != manifest_names[++i]);

  return pkg;
}

/**
 * Build a repo
 */

char *clib_package_get_id(const char *author, const char *name) {
  int size = strlen(author) + 1 // /
             + strlen(name) + 1 // \0
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

static inline list_t *parse_package_deps(JSON_Object *obj) {
  list_t *list = NULL;

  if (!obj)
    goto done;
  if (!(list = list_new()))
    goto done;
  list->free = clib_package_dependency_free;

  for (unsigned int i = 0; i < json_object_get_count(obj); i++) {
    const char *name = NULL;
    char *version = NULL;
    clib_package_dependency_t *dep = NULL;
    int error = 1;

    if (!(name = json_object_get_name(obj, i)))
      goto loop_cleanup;
    if (!(version = json_object_get_string_safe(obj, name)))
      goto loop_cleanup;
    if (!(dep = clib_package_dependency_new(name, version)))
      goto loop_cleanup;
    if (!(list_rpush(list, list_node_new(dep))))
      goto loop_cleanup;

    error = 0;

  loop_cleanup:
    if (version)
      free(version);
    if (error) {
      list_destroy(list);
      list = NULL;
      break;
    }
  }

done:
  return list;
}

/**
 * Create a new clib package from the given `json`
 */

clib_package_t *clib_package_new(const char *json, int verbose) {
  clib_package_t *pkg = NULL;
  JSON_Value *root = NULL;
  JSON_Object *json_object = NULL;
  JSON_Array *src = NULL;
  JSON_Object *deps = NULL;
  JSON_Object *devs = NULL;
  int error = 1;

  if (!json) {
    if (verbose) {
      logger_error("error", "missing JSON to parse");
    }
    goto cleanup;
  }

  if (!(root = json_parse_string(json))) {
    if (verbose) {
      logger_error("error", "unable to parse JSON");
    }
    goto cleanup;
  }

  if (!(json_object = json_value_get_object(root))) {
    if (verbose) {
      logger_error("error", "invalid clib.json or package.json file");
    }
    goto cleanup;
  }

  if (!(pkg = malloc(sizeof(clib_package_t)))) {
    goto cleanup;
  }

  memset(pkg, 0, sizeof(clib_package_t));

  pkg->json = strdup(json);
  pkg->name = json_object_get_string_safe(json_object, "name");
  pkg->repo = json_object_get_string_safe(json_object, "repo");
  pkg->version = json_object_get_string_safe(json_object, "version");
  pkg->license = json_object_get_string_safe(json_object, "license");
  pkg->description = json_object_get_string_safe(json_object, "description");
  pkg->configure = json_object_get_string_safe(json_object, "configure");
  pkg->install = json_object_get_string_safe(json_object, "install");
  pkg->makefile = json_object_get_string_safe(json_object, "makefile");
  pkg->prefix = json_object_get_string_safe(json_object, "prefix");
  pkg->flags = json_object_get_string_safe(json_object, "flags");

  if (!pkg->flags) {
    pkg->flags = json_object_get_string_safe(json_object, "cflags");
  }

  // try as array
  if (!pkg->flags) {
    JSON_Array *flags = json_object_get_array(json_object, "flags");

    if (!flags) {
      flags = json_object_get_array(json_object, "cflags");
    }

    if (flags) {
      for (unsigned int i = 0; i < json_array_get_count(flags); i++) {
        char *flag = json_array_get_string_safe(flags, i);
        if (flag) {
          if (!pkg->flags) {
            pkg->flags = "";
          }

          if (-1 == asprintf(&pkg->flags, "%s %s", pkg->flags, flag)) {
            goto cleanup;
          }

          free(flag);
        }
      }
    }
  }

  if (!pkg->repo && pkg->author && pkg->name) {
    asprintf(&pkg->repo, "%s/%s", pkg->author, pkg->name);
    _debug("creating package: %s", pkg->repo);
  }

  if (!pkg->author) {
    _debug("unable to determine package author for: %s", pkg->name);
  }

  // TODO npm-style "repository" (thlorenz/gumbo-parser.c#1)
  if (pkg->repo) {
    pkg->author = parse_repo_owner(pkg->repo, DEFAULT_REPO_OWNER);
    // repo name may not be package name (thing.c -> thing)
    pkg->repo_name = parse_repo_name(pkg->repo);
  } else {
    if (verbose) {
      logger_warn("warning",
                  "missing repo in clib.json or package.json file for %s",
                  pkg->name);
    }
    pkg->author = NULL;
    pkg->repo_name = NULL;
  }

  src = json_object_get_array(json_object, "src");

  if (!src) {
    src = json_object_get_array(json_object, "files");
  }

  if (src) {
    if (!(pkg->src = list_new()))
      goto cleanup;
    pkg->src->free = free;
    for (unsigned int i = 0; i < json_array_get_count(src); i++) {
      char *file = json_array_get_string_safe(src, i);
      _debug("file: %s", file);
      if (!file)
        goto cleanup;
      if (!(list_rpush(pkg->src, list_node_new(file))))
        goto cleanup;
    }
  } else {
    _debug("no src files listed in clib.json or package.json file");
    pkg->src = NULL;
  }

  if (!(pkg->registries = list_new())) {
    goto cleanup;
  }
  JSON_Array* registries = json_object_get_array(json_object, "registries");
  if (registries) {
    pkg->registries->free = free;
    for (unsigned int i = 0; i < json_array_get_count(registries); i++) {
      char *file = json_array_get_string_safe(registries, i);
      _debug("file: %s", file);
      if (!file)
        goto cleanup;
      if (!(list_rpush(pkg->registries, list_node_new(file))))
        goto cleanup;
    }
  } else {
    _debug("no extra registries listed in clib.json or package.json file");
  }

  if ((deps = json_object_get_object(json_object, "dependencies"))) {
    if (!(pkg->dependencies = parse_package_deps(deps))) {
      goto cleanup;
    }
  } else {
    _debug("no dependencies listed in clib.json or package.json file");
    pkg->dependencies = NULL;
  }

  if ((devs = json_object_get_object(json_object, "development"))) {
    if (!(pkg->development = parse_package_deps(devs))) {
      goto cleanup;
    }
  } else {
    _debug(
        "no development dependencies listed in clib.json or package.json file");
    pkg->development = NULL;
  }

  error = 0;

cleanup:
  if (root)
    json_value_free(root);
  if (error && pkg) {
    clib_package_free(pkg);
    pkg = NULL;
  }
  return pkg;
}

static clib_package_t *
clib_package_new_from_slug_with_package_name(const char *slug, const char* url, int verbose, const char *file) {
  char *author = NULL;
  char *name = NULL;
  char *version = NULL;
  char *json_url = NULL;
  char *repo = NULL;
  char *json = NULL;
  char *log = NULL;
  http_get_response_t *res = NULL;
  clib_package_t *pkg = NULL;
  int retries = 3;

  // parse chunks
  if (!slug)
    goto error;
  _debug("creating package: %s", slug);
  if (!(author = parse_repo_owner(slug, DEFAULT_REPO_OWNER)))
    goto error;
  if (!(name = parse_repo_name(slug)))
    goto error;
  if (!(version = parse_repo_version(slug, DEFAULT_REPO_VERSION)))
    goto error;

  _debug("author: %s", author);
  _debug("name: %s", name);
  _debug("version: %s", version);

#ifdef HAVE_PTHREADS
  pthread_mutex_lock(&lock.mutex);
#endif
  // fetch json
  if (clib_cache_has_json(author, name, version)) {
    if (package_opts.skip_cache) {
      clib_cache_delete_json(author, name, version);
      goto download;
    }

    json = clib_cache_read_json(author, name, version);

    if (!json) {
      goto download;
    }

    log = "cache";
#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&lock.mutex);
#endif
  } else {
  download:
#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&lock.mutex);
#endif
    if (retries-- <= 0) {
      goto error;
    } else {
      _debug("Fetching package manifest for %s", slug);
      // clean up when retrying
      res = repository_fetch_package_manifest(url, slug, version);

      json = res->data;
      _debug("status: %d", res->status);
      if (!res || !res->ok) {
        goto download;
      }
      log = "fetch";
    }
  }

  if (verbose) {
    logger_info(log, "%s/%s:%s", author, name, file);
  }

  free(json_url);
  json_url = NULL;
  free(name);
  name = NULL;

  if (json) {
    // build package
    pkg = clib_package_new(json, verbose);
  }

  // Set the url so that we can download the other files.
  pkg->url = url;

  if (!pkg)
    goto error;

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
  if (author && pkg->author) {
    if (0 != strcmp(author, pkg->author)) {
      free(pkg->author);
      pkg->author = author;
    } else {
      free(author);
    }
  } else {
    pkg->author = strdup(author);
  }

  if (!(repo = clib_package_get_id(pkg->author, pkg->name))) {
    goto error;
  }

#ifdef HAVE_PTHREADS
  pthread_mutex_lock(&lock.mutex);
#endif
  // cache json
  if (pkg && pkg->author && pkg->name && pkg->version) {
    if (-1 ==
        clib_cache_save_json(pkg->author, pkg->name, pkg->version, json)) {
      _debug("failed to cache JSON for: %s/%s@%s", pkg->author, pkg->name,
             pkg->version);
    } else {
      _debug("cached: %s/%s@%s", pkg->author, pkg->name, pkg->version);
    }
  }
#ifdef HAVE_PTHREADS
  pthread_mutex_unlock(&lock.mutex);
#endif

  if (res) {
    http_get_free(res);
    json = NULL;
    res = NULL;
  } else {
    free(json);
    json = NULL;
  }

  return pkg;

error:
  if (0 == retries) {
    if (verbose && author && name && file) {
      logger_warn("warning", "unable to fetch %s/%s:%s", author, name, file);
    }
  }

  free(author);
  free(name);
  free(version);
  free(json_url);
  free(repo);
  if (!res && json)
    free(json);
  if (res)
    http_get_free(res);
  if (pkg)
    clib_package_free(pkg);
  return NULL;
}

/**
 * Create a package from the given repo `slug`
 */
clib_package_t *clib_package_new_from_slug_and_url(const char *slug, const char* url, int verbose) {
  clib_package_t *package = NULL;
  const char *name = NULL;
  unsigned int i = 0;

  do {
    name = manifest_names[i];
    package = clib_package_new_from_slug_with_package_name(slug, url, verbose, name);
    if (NULL != package) {
      package->filename = (char *)name;
    }
  } while (NULL != manifest_names[++i] && NULL == package);

  return package;
}

/**
 * Parse the package author from the given `slug`
 */

char *clib_package_parse_author(const char *slug) {
  return parse_repo_owner(slug, DEFAULT_REPO_OWNER);
}

/**
 * Parse the package version from the given `slug`
 */

char *clib_package_parse_version(const char *slug) {
  return parse_repo_version(slug, DEFAULT_REPO_VERSION);
}

/**
 * Parse the package name from the given `slug`
 */

char *clib_package_parse_name(const char *slug) {
  return parse_repo_name(slug);
}

/**
 * Create a new package dependency from the given `repo` and `version`
 */

clib_package_dependency_t *clib_package_dependency_new(const char *repo,
                                                       const char *version) {
  if (!repo || !version)
    return NULL;

  clib_package_dependency_t *dep = malloc(sizeof(clib_package_dependency_t));
  if (!dep) {
    return NULL;
  }

  dep->version = 0 == strcmp("*", version) ? strdup(DEFAULT_REPO_VERSION)
                                           : strdup(version);
  dep->name = clib_package_parse_name(repo);
  dep->author = clib_package_parse_author(repo);

  _debug("dependency: %s/%s@%s", dep->author, dep->name, dep->version);
  return dep;
}


void clib_package_set_prefix(clib_package_t *pkg, long path_max) {
  if (NULL != package_opts.prefix || NULL != pkg->prefix) {
    char path[path_max];
    memset(path, 0, path_max);

    if (package_opts.prefix) {
      realpath(package_opts.prefix, path);
    } else {
      realpath(pkg->prefix, path);
    }

    _debug("env: PREFIX: %s", path);
    setenv("PREFIX", path, 1);
    mkdirp(path, 0777);
  }
}

/**
 * Free a clib package
 */

void clib_package_free(clib_package_t *pkg) {
  if (NULL == pkg) {
    return;
  }

  if (0 != pkg->refs) {
    return;
  }

#define FREE(k)                                                                \
  if (pkg->k) {                                                                \
    free(pkg->k);                                                              \
    pkg->k = 0;                                                                \
  }
  FREE(author);
  FREE(description);
  FREE(install);
  FREE(json);
  FREE(license);
  FREE(name);
  FREE(makefile);
  FREE(configure);
  FREE(repo);
  FREE(repo_name);
  FREE(url);
  FREE(version);
  FREE(flags);
#undef FREE

  if (pkg->src)
    list_destroy(pkg->src);
  pkg->src = 0;

  if (pkg->dependencies)
    list_destroy(pkg->dependencies);
  pkg->dependencies = 0;

  if (pkg->development)
    list_destroy(pkg->development);
  pkg->development = 0;

  free(pkg);
  pkg = 0;
}

void clib_package_dependency_free(void *_dep) {
  clib_package_dependency_t *dep = (clib_package_dependency_t *)_dep;
  free(dep->name);
  free(dep->author);
  free(dep->version);
  free(dep);
}

void clib_package_cleanup() {
  if (0 != visited_packages) {
    hash_each(visited_packages, {
      free((void *)key);
      (void)val;
    });

    hash_free(visited_packages);
    visited_packages = 0;
  }
}
