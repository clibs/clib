
//
// clib-package.c
//
// Copyright (c) 2014 Stephen Mathieson
// MIT license
//


#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#include <unistd.h>
#endif

#include <limits.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>
#include "asprintf/asprintf.h"
#include "tempdir/tempdir.h"
#include "strdup/strdup.h"
#include "parson/parson.h"
#include "substr/substr.h"
#include "http-get/http-get.h"
#include "mkdirp/mkdirp.h"
#include "path-join/path-join.h"
#include "logger/logger.h"
#include "parse-repo/parse-repo.h"
#include "debug/debug.h"
#include "clib-package.h"
#include "clib-cache/cache.h"
#include "fs/fs.h"

#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif

#ifndef DEFAULT_REPO_VERSION
#define DEFAULT_REPO_VERSION "master"
#endif

#ifndef DEFAULT_REPO_OWNER
#define DEFAULT_REPO_OWNER "clibs"
#endif

#define GITHUB_CONTENT_URL "https://raw.githubusercontent.com/"

#ifdef HAVE_PTHREADS
typedef struct fetch_package_file_thread_data fetch_package_file_thread_data_t;
struct fetch_package_file_thread_data {
  clib_package_t *pkg;
  const char *dir;
  char *file;
  int verbose;
  pthread_t thread;
  pthread_attr_t attr;
  void *data;
};

typedef struct clib_package_lock clib_package_lock_t;
struct clib_package_lock {
  pthread_mutex_t mutex;
};

static clib_package_lock_t lock = {
  PTHREAD_MUTEX_INITIALIZER
};

#endif

CURLSH *clib_package_curl_share;
debug_t _debugger;

#define _debug(...) ({                                           \
  if (!(_debugger.name)) debug_init(&_debugger, "clib-package"); \
  debug(&_debugger, __VA_ARGS__);                                \
})

#define E_FORMAT(...) ({      \
  rc = asprintf(__VA_ARGS__); \
  if (-1 == rc) goto cleanup; \
});

static clib_package_opts_t opts = {
   .skip_cache = 1,
   .prefix = 0,
   .force = 0,
};

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


void
clib_package_set_opts(clib_package_opts_t o) {
  if (1 == opts.skip_cache && 0 == o.skip_cache) {
    opts.skip_cache = 0;
  } else if (0 == opts.skip_cache && 1 == o.skip_cache) {
    opts.skip_cache = 1;
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
}

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
    memset(res, 0, size);
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

  list_t *freelist = list_new();

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

    list_rpush(freelist, list_node_new(pkg));
    error = 0;

  loop_cleanup:
    if (slug) free(slug);
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
  iterator = list_iterator_new(freelist, LIST_HEAD);
  while ((node = list_iterator_next(iterator))) {
    clib_package_t *pkg = node->val;
    if (pkg) clib_package_free(pkg);
  }
  list_iterator_destroy(iterator);
  list_destroy(freelist);
  return rc;
}

#ifdef HAVE_PTHREADS
static void
curl_lock_callback(
    CURL *handle
  , curl_lock_data data
  , curl_lock_access access
  , void *userptr
) {
  pthread_mutex_lock(&lock.mutex);
}

static void
curl_unlock_callback(
    CURL *handle
  , curl_lock_data data
  , curl_lock_access access
  , void *userptr
) {
  pthread_mutex_unlock(&lock.mutex);
}

static void
init_curl_share() {
  if (0 == clib_package_curl_share) {
    pthread_mutex_lock(&lock.mutex);
    clib_package_curl_share = curl_share_init();
    curl_share_setopt(clib_package_curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
    curl_share_setopt(clib_package_curl_share, CURLSHOPT_LOCKFUNC, curl_lock_callback);
    curl_share_setopt(clib_package_curl_share, CURLSHOPT_UNLOCKFUNC, curl_unlock_callback);
    pthread_mutex_unlock(&lock.mutex);
  }
}
#endif

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
    logger_error("error", "invalid clib.json or package.json file");
    goto cleanup;
  }
  if (!(pkg = malloc(sizeof(clib_package_t)))) goto cleanup;

  memset(pkg, 0, sizeof(clib_package_t));

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
    if (verbose) logger_warn("warning", "missing repo in clib.json or package.json file");
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
    _debug("no src files listed in clib.json or package.json file");
    pkg->src = NULL;
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
    _debug("no development dependencies listed in clib.json or package.json file");
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

static clib_package_t *
clib_package_new_from_slug_with_package_name(const char *slug, int verbose, const char *file) {
  char *author = NULL;
  char *name = NULL;
  char *version = NULL;
  char *url = NULL;
  char *json_url = NULL;
  char *repo = NULL;
  char *json = NULL;
  char *log = NULL;
  http_get_response_t *res = NULL;
  clib_package_t *pkg = NULL;
  int retries = 3;

  // parse chunks
  if (!slug) goto error;
  _debug("creating package: %s", slug);
  if (!(author = parse_repo_owner(slug, DEFAULT_REPO_OWNER))) goto error;
  if (!(name = parse_repo_name(slug))) goto error;
  if (!(version = parse_repo_version(slug, DEFAULT_REPO_VERSION))) goto error;
  if (!(url = clib_package_url(author, name, version))) goto error;
  if (!(json_url = clib_package_file_url(url, file))) goto error;

  _debug("author: %s", author);
  _debug("name: %s", name);
  _debug("version: %s", version);

  // fetch json
  if (clib_cache_has_json(author, name, version)) {
    if (opts.skip_cache) {
      clib_cache_delete_json(author, name, version);
      goto download;
    }
    json = clib_cache_read_json(author, name, version);
    if (!json){
      goto download;
    }
    log = "cache";
  } else {
download:
    if (retries-- <= 0) {
      goto error;
    } else {
#ifdef HAVE_PTHREADS
      init_curl_share();
      _debug("GET %s", json_url);
      res = http_get_shared(json_url, clib_package_curl_share);
#else
      res = http_get(json_url);
#endif
      json = res->data;
      _debug("status: %d", res->status);
      if (!res || !res->ok) {
        logger_warn("warning", "unable to fetch %s/%s:%s", author, name, file);
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

  // build package
  pkg = clib_package_new(json, verbose);
  if (res) {
    http_get_free(res);
  } else {
    free(json);
  }

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
  if (author && pkg->author) {
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
 * Create a package from the given repo `slug`
 */

clib_package_t *
clib_package_new_from_slug(const char *slug, int verbose) {
  clib_package_t *package = NULL;
  const char *name = NULL;
  unsigned int i = 0;

  static const char *package_names[] = {
    "clib.json",
    "package.json",
    NULL
  };

  do {
    name = package_names[i];
    package = clib_package_new_from_slug_with_package_name(slug, verbose, name);
    if (NULL != package) {
      package->filename = (char *) name;
    }
  } while (NULL != package_names[++i] && NULL == package);

  return package;
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

static int
fetch_package_file_work(
    clib_package_t *pkg
  , const char *dir
  , char *file
  , int verbose
) {
  char *url = NULL;
  char *path = NULL;
  int saved = 0;
  int rc = 0;

  _debug("fetch file: %s/%s", pkg->repo, file);

  if (NULL == pkg) {
    return 1;
  }

  if (NULL == pkg->url) {
    return 1;
  }

  if (0 == strncmp(file, "http", 4)) {
    url = strdup(file);
  } else if (!(url = clib_package_file_url(pkg->url, file))) {
    return 1;
  }

  _debug("file URL: %s", url);

  if (!(path = path_join(dir, basename(file)))) {
    rc = 1;
    goto cleanup;
  }

#ifdef HAVE_PTHREADS
      pthread_mutex_lock(&lock.mutex);
#endif

  if (1 == opts.force || -1 == fs_exists(path)) {
    if (verbose) {
      logger_info("fetch", "%s:%s", pkg->repo, file);
      fflush(stdout);
    }

#ifdef HAVE_PTHREADS
      pthread_mutex_unlock(&lock.mutex);
#endif

    rc = http_get_file_shared(url, path, clib_package_curl_share);
    saved = 1;
  } else {
#ifdef HAVE_PTHREADS
      pthread_mutex_unlock(&lock.mutex);
#endif
  }


  if (-1 == rc) {
#ifdef HAVE_PTHREADS
  pthread_mutex_lock(&lock.mutex);
#endif
    logger_error("error", "unable to fetch %s:%s", pkg->repo, file);
    fflush(stderr);
    rc = 1;
#ifdef HAVE_PTHREADS
  pthread_mutex_unlock(&lock.mutex);
#endif
    goto cleanup;
  }

  if (saved) {
    if (verbose) {
#ifdef HAVE_PTHREADS
      pthread_mutex_lock(&lock.mutex);
#endif
      logger_info("save", path);
      fflush(stdout);
#ifdef HAVE_PTHREADS
      pthread_mutex_unlock(&lock.mutex);
#endif
    }
  }

cleanup:

  free(url);
  free(path);
  return rc;
}

#ifdef HAVE_PTHREADS
static void *
fetch_package_file_thread(void *arg) {
  fetch_package_file_thread_data_t *data = arg;
  int rc = fetch_package_file_work(
      data->pkg
    , data->dir
    , data->file
    , data->verbose);
  (void) data->pkg->refs--;
  pthread_exit((void *) rc);
  return (void *) rc;
}
#endif

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
  , void **data
) {
#ifndef HAVE_PTHREADS
  return fetch_package_file_work(pkg, dir, file, verbose);
#else
  fetch_package_file_thread_data_t *fetch = malloc(sizeof(*fetch));
  int rc = 0;

  if (0 == fetch) {
    return -1;
  }

  *data = 0;

  memset(fetch, 0, sizeof(*fetch));

  fetch->pkg = pkg;
  fetch->dir = dir;
  fetch->file = file;
  fetch->verbose = verbose;

  rc = pthread_attr_init(&fetch->attr);

  if (0 != rc) {
    free(fetch);
    return rc;
  }

  (void) pkg->refs++;
  rc = pthread_create(
      &fetch->thread
    , NULL
    , fetch_package_file_thread
    , fetch);

  if (0 != rc) {
    pthread_attr_destroy(&fetch->attr);
    free(fetch);
    return rc;
  }

  rc = pthread_attr_destroy(&fetch->attr);

  if (0 != rc) {
    pthread_cancel(fetch->thread);
    free(fetch);
    return rc;
  }

  *data = fetch;

  return rc;
#endif
}

int
clib_package_install_executable(clib_package_t *pkg , char *dir, int verbose) {
  int rc;
  char *url = NULL;
  char *file = NULL;
  char *tarball = NULL;
  char *command = NULL;
  char *unpack_dir = NULL;
  char *deps = NULL;
  char *tmp = NULL;
  char *reponame = NULL;

  _debug("install executable %s", pkg->repo);

  tmp = gettempdir();
  if (NULL == tmp) {
    logger_error("error", "gettempdir() out of memory");
    return -1;
  }

  if (!pkg->repo) {
    logger_error("error", "repo field required to install executable");
    return -1;
  }

  reponame = strrchr(pkg->repo, '/');
  if (reponame && *reponame != '\0') reponame++;
  else {
    logger_error(
      "error",
      "malformed repo field, must be in the form user/pkg");
    return -1;
  }

  E_FORMAT(&url
    , "https://github.com/%s/archive/%s.tar.gz"
    , pkg->repo
    , pkg->version);

  E_FORMAT(&file
    , "%s-%s.tar.gz"
    , reponame
    , pkg->version);

  E_FORMAT(&tarball
    , "%s/%s"
    , tmp, file);

  rc = http_get_file_shared(url, tarball, clib_package_curl_share);

  if (0 != rc) {
    logger_error("error"
      , "download failed for '%s@%s' - HTTP GET '%s'"
      , pkg->repo
      , pkg->version
      , url);

    goto cleanup;
  }

  E_FORMAT(&command, "cd %s && gzip -dc %s | tar x", tmp, file);

  _debug("download url: %s", url);
  _debug("file: %s", file);
  _debug("tarball: %s", tarball);
  _debug("command: %s", command);

  // cheap untar
  rc = system(command);
  if (0 != rc) goto cleanup;

  char *version = pkg->version;
  if ('v' == version[0]) {
    (void) version++;
  }

  E_FORMAT(&unpack_dir, "%s/%s-%s", tmp, reponame, version);

  _debug("dir: %s", unpack_dir);

  if (pkg->dependencies) {
    E_FORMAT(&deps, "%s/deps", unpack_dir);
    _debug("deps: %s", deps);
    rc = clib_package_install_dependencies(pkg, deps, verbose);
    if (-1 == rc) goto cleanup;
  }

  free(command);
  command = NULL;

  if (NULL != opts.prefix) {
    char path[PATH_MAX] = { 0 };
    realpath(opts.prefix, path);
    _debug("env: PREFIX: %s", path);
    setenv("PREFIX", path, 1);
  }

  char dir_path[PATH_MAX] = { 0 };
  realpath(dir, dir_path);
  E_FORMAT(&command
      , "cp -fr %s/%s/%s %s && cd %s && %s"
      , dir_path
      , pkg->name
      , basename(pkg->makefile)
      , unpack_dir
      , unpack_dir
      , pkg->install);

  _debug("command: %s", command);
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

int
clib_package_install(clib_package_t *pkg, const char *dir, int verbose) {
  list_iterator_t *iterator = NULL;
  char *package_json = NULL;
  char *pkg_dir = NULL;
  int pending = 0;
  int max = 4;
  int rc = -1;
  int i = 0;

#ifdef HAVE_PTHREADS
    fetch_package_file_thread_data_t **fetchs = 0;
    if (NULL != pkg && NULL != pkg->src) {
      if (pkg->src->len > 0) {
        fetchs = malloc(pkg->src->len * sizeof(fetch_package_file_thread_data_t));
      }
    }

    if (fetchs) {
      memset(fetchs, 0, pkg->src->len * sizeof(fetch_package_file_thread_data_t));
    }

#endif

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

  // write clib.json or package.json
  if (!(package_json = path_join(pkg_dir, pkg->filename))) goto cleanup;
  _debug("write: %s", package_json);
  if (-1 == fs_write(package_json, pkg->json)) {
    logger_error("error", "Failed to write %s", package_json);
    goto cleanup;
  }

  // fetch makefile
  if (pkg->makefile) {
    _debug("fetch: %s/%s", pkg->repo, pkg->makefile);
    void *fetch = 0;
    rc = fetch_package_file(pkg, pkg_dir, pkg->makefile, verbose, &fetch);
    if (0 != rc) {
      goto cleanup;
    }

#ifdef HAVE_PTHREADS
    if (0 != fetch) {
      fetch_package_file_thread_data_t *data = fetch;
      int *status = 0;
      pthread_join(data->thread, (void **) &status);
      if (0 != status) {
        rc = *status;
        status = 0;
      }
    }
#endif
  }

  // if no sources are listed, just install
  if (NULL == pkg->src) goto install;

  if (clib_cache_has_package(pkg->author, pkg->name, pkg->version)) {
    if (opts.skip_cache) {
        clib_cache_delete_package(pkg->author, pkg->name, pkg->version);
        goto download;
    }

    if (0 != clib_cache_load_package(pkg->author, pkg->name, pkg->version, pkg_dir)){
      goto download;
    }

    logger_info("cache", pkg->repo);
    goto install;
  }

download:
  iterator = list_iterator_new(pkg->src, LIST_HEAD);
  list_node_t *source;

  while ((source = list_iterator_next(iterator))) {
    void *fetch = NULL;
    rc = fetch_package_file(pkg, pkg_dir, source->val, verbose, &fetch);

    if (0 != rc) {
      list_iterator_destroy(iterator);
      iterator = NULL;
      rc = -1;
      goto cleanup;
    }

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
    usleep(1024 * 10);
#endif

#ifdef HAVE_PTHREADS
    if (i < 0) {
      i = 0;
    }

    fetchs[i] = fetch;

    (void) pending++;

    if (i < max) {
      (void) i++;
    } else {
      while (--i >= 0) {
        fetch_package_file_thread_data_t *data = fetchs[i];
        int *status = 0;
        pthread_join(data->thread, (void **) status);
        free(data);
        fetchs[i] = NULL;

        (void) pending--;

        if (0 != status) {
          rc = *status;
          status = 0;
        }

        if (0 != rc) {
          rc = -1;
          goto cleanup;
        }

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
        usleep(1024 * 10);
#endif
      }
    }
#endif
  }

#ifdef HAVE_PTHREADS
  while (--i >= 0) {
    fetch_package_file_thread_data_t *data = fetchs[i];
    int *status = 0;

    pthread_join(data->thread, (void **) status);

    (void) pending--;
    free(data);
    fetchs[i] = NULL;

    if (0 != status) {
      rc = *status;
      status = 0;
    }

    if (0 != rc) {
      rc = -1;
      goto cleanup;
    }
  }
#endif

  clib_cache_save_package(pkg->author, pkg->name, pkg->version, pkg_dir);

install:
  if (pkg->install) {
    rc = clib_package_install_executable(pkg, dir, verbose);
  } else {
    rc = 0;
  }

  if (0 != rc) {
    goto cleanup;
  }

  rc = clib_package_install_dependencies(pkg, dir, verbose);

cleanup:
  if (pkg_dir) free(pkg_dir);
  if (package_json) free(package_json);
  if (iterator) list_iterator_destroy(iterator);
#ifdef HAVE_PTHREADS
  if (NULL != pkg && NULL != pkg->src) {
    if (pkg->src->len > 0) {
      if (fetchs) {
        free(fetchs);
      }
    }
  }
  fetchs = NULL;
#endif
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
  if (NULL == pkg) {
    return;
  }

  if (0 != pkg->refs) {
    return;
  }

  free(pkg->author);
  pkg->author = 0;

  free(pkg->description);
  pkg->description = 0;

  free(pkg->install);
  pkg->install = 0;

  free(pkg->json);
  pkg->json = 0;

  free(pkg->license);
  pkg->license = 0;

  free(pkg->name);
  pkg->name = 0;

  free(pkg->makefile);
  pkg->makefile = 0;

  free(pkg->repo);
  pkg->repo = 0;

  free(pkg->repo_name);
  pkg->repo_name = 0;

  free(pkg->url);
  pkg->url = 0;

  free(pkg->version);
  pkg->version = 0;

  if (pkg->src) list_destroy(pkg->src);
  pkg->src = 0;

  if (pkg->dependencies) list_destroy(pkg->dependencies);
  pkg->dependencies = 0;

  if (pkg->development) list_destroy(pkg->development);
  pkg->development = 0;

  free(pkg);
  pkg = 0;
}

void
clib_package_dependency_free(void *_dep) {
  clib_package_dependency_t *dep = (clib_package_dependency_t *) _dep;
  free(dep->name);
  free(dep->author);
  free(dep->version);
  free(dep);
}
