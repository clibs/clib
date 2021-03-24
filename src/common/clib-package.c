// clib-package.c
//
// Copyright (c) 2014 Stephen Mathieson
// Copyright (c) 2014-2020 clib authors
// MIT license
//

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif

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
#include "path-join/path-join.h"
#include "strdup/strdup.h"
#include "substr/substr.h"
#include "tempdir/tempdir.h"
#include <curl/curl.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
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

CURLSH *clib_package_curl_share;
debug_t _debugger;

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

static clib_package_opts_t opts = {
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

static inline char *clib_package_slug(const char *, const char *, const char *);

static inline char *clib_package_repo(const char *, const char *);

static inline list_t *parse_package_deps(JSON_Object *);

static inline int install_packages(list_t *, const char *, int);

void clib_package_set_opts(clib_package_opts_t o) {
  if (1 == opts.skip_cache && 0 == o.skip_cache) {
    opts.skip_cache = 0;
  } else if (0 == opts.skip_cache && 1 == o.skip_cache) {
    opts.skip_cache = 1;
  }

  if (1 == opts.global && 0 == o.global) {
    opts.global = 0;
  } else if (0 == opts.global && 1 == o.global) {
    opts.global = 1;
  }

  if (1 == opts.force && 0 == o.force) {
    opts.force = 0;
  } else if (0 == opts.force && 1 == o.force) {
    opts.force = 1;
  }

  if (0 != o.prefix) {
    if (0 == strlen(o.prefix)) {
      opts.prefix = 0;
    } else {
      opts.prefix = o.prefix;
    }
  }

  if (0 != o.token) {
    if (0 == strlen(o.token)) {
      opts.token = 0;
    } else {
      opts.token = o.token;
    }
  }

  if (o.concurrency) {
    opts.concurrency = o.concurrency;
  } else if (o.concurrency < 0) {
    opts.concurrency = 0;
  }

  if (opts.concurrency < 0) {
    opts.concurrency = 0;
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

static inline char *clib_package_slug(const char *author, const char *name,
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

static inline char *clib_package_repo(const char *author, const char *name) {
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

static inline int install_packages(list_t *list, const char *dir, int verbose) {
  list_node_t *node = NULL;
  list_iterator_t *iterator = NULL;
  int rc = -1;
  list_t *freelist = NULL;

  if (!list || !dir)
    goto cleanup;

  iterator = list_iterator_new(list, LIST_HEAD);
  if (NULL == iterator)
    goto cleanup;

  freelist = list_new();

  while ((node = list_iterator_next(iterator))) {
    clib_package_dependency_t *dep = NULL;
    char *slug = NULL;
    clib_package_t *pkg = NULL;
    int error = 1;

    dep = (clib_package_dependency_t *)node->val;
    slug = clib_package_slug(dep->author, dep->name, dep->version);
    if (NULL == slug)
      goto loop_cleanup;

    pkg = clib_package_new_from_slug_and_url(slug, "FIXME", verbose);
    if (NULL == pkg)
      goto loop_cleanup;

    if (-1 == clib_package_install(pkg, dir, verbose))
      goto loop_cleanup;

    list_rpush(freelist, list_node_new(pkg));
    error = 0;

  loop_cleanup:
    if (slug)
      free(slug);
    if (error) {
      list_iterator_destroy(iterator);
      iterator = NULL;
      rc = -1;
      goto cleanup;
    }
  }

  rc = 0;

cleanup:
  if (iterator)
    list_iterator_destroy(iterator);

  if (freelist) {
    iterator = list_iterator_new(freelist, LIST_HEAD);
    while ((node = list_iterator_next(iterator))) {
      clib_package_t *pkg = node->val;
      if (pkg)
        clib_package_free(pkg);
    }
    list_iterator_destroy(iterator);
    list_destroy(freelist);
  }
  return rc;
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
    if (opts.skip_cache) {
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

  if (!(repo = clib_package_repo(pkg->author, pkg->name))) {
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


static void set_prefix(clib_package_t *pkg, long path_max) {
  if (NULL != opts.prefix || NULL != pkg->prefix) {
    char path[path_max];
    memset(path, 0, path_max);

    if (opts.prefix) {
      realpath(opts.prefix, path);
    } else {
      realpath(pkg->prefix, path);
    }

    _debug("env: PREFIX: %s", path);
    setenv("PREFIX", path, 1);
    mkdirp(path, 0777);
  }
}

int clib_package_install_executable(clib_package_t *pkg, const char *dir,
                                    int verbose) {
#ifdef PATH_MAX
  long path_max = PATH_MAX;
#elif defined(_PC_PATH_MAX)
  long path_max = pathconf(dir, _PC_PATH_MAX);
#else
  long path_max = 4096;
#endif

  int rc;
  char *url = NULL;
  char *file = NULL;
  char *tarball = NULL;
  char *command = NULL;
  char *unpack_dir = NULL;
  char *deps = NULL;
  char *tmp = NULL;
  char *reponame = NULL;
  char dir_path[path_max];

  _debug("install executable %s", pkg->repo);

  tmp = gettempdir();

  if (NULL == tmp) {
    if (verbose) {
      logger_error("error", "gettempdir() out of memory");
    }
    return -1;
  }

  if (!pkg->repo) {
    if (verbose) {
      logger_error("error", "repo field required to install executable");
    }
    return -1;
  }

  reponame = strrchr(pkg->repo, '/');
  if (reponame && *reponame != '\0')
    reponame++;
  else {
    if (verbose) {
      logger_error("error",
                   "malformed repo field, must be in the form user/pkg");
    }
    return -1;
  }

  E_FORMAT(&url, "https://github.com/%s/archive/%s.tar.gz", pkg->repo,
           pkg->version);

  E_FORMAT(&file, "%s-%s.tar.gz", reponame, pkg->version);

  E_FORMAT(&tarball, "%s/%s", tmp, file);

  rc = http_get_file_shared(url, tarball, clib_package_curl_share, NULL, 0);

  if (0 != rc) {
    if (verbose) {
      logger_error("error", "download failed for '%s@%s' - HTTP GET '%s'",
                   pkg->repo, pkg->version, url);
    }

    goto cleanup;
  }

  E_FORMAT(&command, "cd %s && gzip -dc %s | tar x", tmp, file);

  _debug("download url: %s", url);
  _debug("file: %s", file);
  _debug("tarball: %s", tarball);
  _debug("command(extract): %s", command);

  // cheap untar
  rc = system(command);
  if (0 != rc)
    goto cleanup;

  free(command);
  command = NULL;

  set_prefix(pkg, path_max);

  const char *configure = pkg->configure;

  if (0 == configure) {
    configure = ":";
  }

  memset(dir_path, 0, path_max);
  realpath(dir, dir_path);

  char *version = pkg->version;
  if ('v' == version[0]) {
    (void)version++;
  }

  E_FORMAT(&unpack_dir, "%s/%s-%s", tmp, reponame, version);

  _debug("dir: %s", unpack_dir);

  if (pkg->dependencies) {
    E_FORMAT(&deps, "%s/deps", unpack_dir);
    _debug("deps: %s", deps);
    rc = clib_package_install_dependencies(pkg, deps, verbose);
    if (-1 == rc)
      goto cleanup;
  }

  if (!opts.global && pkg->makefile) {
    E_FORMAT(&command, "cp -fr %s/%s/%s %s", dir_path, pkg->name,
             basename(pkg->makefile), unpack_dir);

    rc = system(command);
    if (0 != rc) {
      goto cleanup;
    }

    free(command);
  }

  if (pkg->flags) {
    char *flags = NULL;
#ifdef _GNU_SOURCE
    char *cflags = secure_getenv("CFLAGS");
#else
    char *cflags = getenv("CFLAGS");
#endif

    if (cflags) {
      asprintf(&flags, "%s %s", cflags, pkg->flags);
    } else {
      asprintf(&flags, "%s", pkg->flags);
    }

    setenv("CFLAGS", cflags, 1);
  }

  E_FORMAT(&command, "cd %s && %s", unpack_dir, pkg->install);

  _debug("command(install): %s", command);
  rc = system(command);

cleanup:
  free(tmp);
  free(command);
  free(tarball);
  free(file);
  free(url);
  return rc;
}

/**
 * Install the given `pkg` in `dir`
 */

int clib_package_install(clib_package_t *pkg, const char *dir, int verbose) {
  list_iterator_t *iterator = NULL;
  char *package_json = NULL;
  char *pkg_dir = NULL;
  char *command = NULL;
  int pending = 0;
  int rc = 0;
  int i = 0;

#ifdef PATH_MAX
  long path_max = PATH_MAX;
#elif defined(_PC_PATH_MAX)
  long path_max = pathconf(dir, _PC_PATH_MAX);
#else
  long path_max = 4096;
#endif

#ifdef HAVE_PTHREADS
  int max = opts.concurrency;
#endif

#ifdef CLIB_PACKAGE_PREFIX
  if (0 == opts.prefix) {
#ifdef HAVE_PTHREADS
    pthread_mutex_lock(&lock.mutex);
#endif
    opts.prefix = CLIB_PACKAGE_PREFIX;
#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&lock.mutex);
#endif
  }
#endif

  if (0 == opts.prefix) {
#ifdef HAVE_PTHREADS
    pthread_mutex_lock(&lock.mutex);
#endif
#ifdef _GNU_SOURCE
    char *prefix = secure_getenv("PREFIX");
#else
    char *prefix = getenv("PREFIX");
#endif

    if (prefix) {
      opts.prefix = prefix;
    }
#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&lock.mutex);
#endif
  }

  if (0 == visited_packages) {
#ifdef HAVE_PTHREADS
    pthread_mutex_lock(&lock.mutex);
#endif

    visited_packages = hash_new();
    // initial write because sometimes `hash_set()` crashes
    hash_set(visited_packages, strdup(""), "");

#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&lock.mutex);
#endif
  }

  if (0 == opts.force && pkg && pkg->name) {
#ifdef HAVE_PTHREADS
    pthread_mutex_lock(&lock.mutex);
#endif

    if (hash_has(visited_packages, pkg->name)) {
#ifdef HAVE_PTHREADS
      pthread_mutex_unlock(&lock.mutex);
#endif
      return 0;
    }

#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&lock.mutex);
#endif
  }

  if (!pkg || !dir) {
    rc = -1;
    goto cleanup;
  }

  set_prefix(pkg, path_max);

  if (!(pkg_dir = path_join(dir, pkg->name))) {
    rc = -1;
    goto cleanup;
  }

  if (!opts.global) {
    _debug("mkdir -p %s", pkg_dir);
    // create directory for pkg
    if (-1 == mkdirp(pkg_dir, 0777)) {
      rc = -1;
      goto cleanup;
    }
  }

  /*
  if (NULL == pkg->url) {
    pkg->url = clib_package_url(pkg->author, pkg->repo_name, pkg->version);

    if (NULL == pkg->url) {
      rc = -1;
      goto cleanup;
    }
  }
   */

  // write clib.json or package.json
  if (!(package_json = path_join(pkg_dir, pkg->filename))) {
    rc = -1;
    goto cleanup;
  }

  if (!opts.global && NULL != pkg->src) {
    _debug("write: %s", package_json);
    if (-1 == fs_write(package_json, pkg->json)) {
      if (verbose) {
        logger_error("error", "Failed to write %s", package_json);
      }

      rc = -1;
      goto cleanup;
    }
  }

  if (pkg->name) {
#ifdef HAVE_PTHREADS
    pthread_mutex_lock(&lock.mutex);
#endif
    if (!hash_has(visited_packages, pkg->name)) {
      hash_set(visited_packages, strdup(pkg->name), "t");
    }
#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&lock.mutex);
#endif
  }

  // fetch makefile
  if (!opts.global && pkg->makefile) {
    _debug("fetch: %s/%s", pkg->repo, pkg->makefile);
    repository_file_handle_t handle = repository_download_package_file(pkg->url, pkg_dir, pkg->version, pkg->makefile, pkg_dir);
    if (handle == NULL) {
      goto cleanup;
    }

#ifdef HAVE_PTHREADS
    repository_file_finish_download(handle);
    repository_file_free(handle);
#endif
  }

  // if no sources are listed, just install
  if (opts.global || NULL == pkg->src)
    goto install;

#ifdef HAVE_PTHREADS
  pthread_mutex_lock(&lock.mutex);
#endif

  if (clib_cache_has_package(pkg->author, pkg->name, pkg->version)) {
    if (opts.skip_cache) {
      clib_cache_delete_package(pkg->author, pkg->name, pkg->version);
#ifdef HAVE_PTHREADS
      pthread_mutex_unlock(&lock.mutex);
#endif
      goto download;
    }

    if (0 != clib_cache_load_package(pkg->author, pkg->name, pkg->version,
                                     pkg_dir)) {
#ifdef HAVE_PTHREADS
      pthread_mutex_unlock(&lock.mutex);
#endif
      goto download;
    }

    if (verbose) {
      logger_info("cache", pkg->repo);
    }

#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&lock.mutex);
#endif

    goto install;
  }

#ifdef HAVE_PTHREADS
  pthread_mutex_unlock(&lock.mutex);
#endif

download:

  iterator = list_iterator_new(pkg->src, LIST_HEAD);
  list_node_t *source;
  repository_file_handle_t* handles = malloc(pkg->src->len*sizeof(repository_file_handle_t));
  while ((source = list_iterator_next(iterator))) {
    handles[i] = repository_download_package_file(pkg->url, pkg_dir, pkg->version, source->val, pkg_dir);

    if (handles[i] == NULL) {
      list_iterator_destroy(iterator);
      iterator = NULL;
      rc = -1;
      goto cleanup;
    }

#ifdef HAVE_PTHREADS
    if (i < 0) {
      i = 0;
    }

    (void)pending++;

    if (i < (max - 1)) {
      (void)i++;
    } else {
      while (--i >= 0) {
        repository_file_finish_download(handles[i]);
        repository_file_free(handles[i]);
        (void)pending--;

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
        usleep(1024 * 10);
#endif
      }
      i = 0;
    }
#endif
  }

#ifdef HAVE_PTHREADS
  while (--i >= 0) {
    repository_file_finish_download(handles[i]);

    (void)pending--;
    repository_file_free(handles[i]);
  }
#endif

#ifdef HAVE_PTHREADS
  pthread_mutex_lock(&lock.mutex);
#endif
  clib_cache_save_package(pkg->author, pkg->name, pkg->version, pkg_dir);
#ifdef HAVE_PTHREADS
  pthread_mutex_unlock(&lock.mutex);
#endif

install:
  if (pkg->configure) {
    E_FORMAT(&command, "cd %s/%s && %s", dir, pkg->name, pkg->configure);

    _debug("command(configure): %s", command);

    rc = system(command);
    if (0 != rc)
      goto cleanup;
  }

  if (0 == rc && pkg->install) {
    rc = clib_package_install_executable(pkg, dir, verbose);
  }

  if (0 == rc) {
    rc = clib_package_install_dependencies(pkg, dir, verbose);
  }

cleanup:
  if (pkg_dir)
    free(pkg_dir);
  if (package_json)
    free(package_json);
  if (iterator)
    list_iterator_destroy(iterator);
  if (command)
    free(command);
  return rc;
}

/**
 * Install the given `pkg`'s dependencies in `dir`
 */

int clib_package_install_dependencies(clib_package_t *pkg, const char *dir,
                                      int verbose) {
  if (!pkg || !dir)
    return -1;
  if (NULL == pkg->dependencies)
    return 0;

  return install_packages(pkg->dependencies, dir, verbose);
}

/**
 * Install the given `pkg`'s development dependencies in `dir`
 */

int clib_package_install_development(clib_package_t *pkg, const char *dir,
                                     int verbose) {
  if (!pkg || !dir)
    return -1;
  if (NULL == pkg->development)
    return 0;

  return install_packages(pkg->development, dir, verbose);
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

  curl_share_cleanup(clib_package_curl_share);
}
