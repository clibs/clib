//
// clib-package-installer.c
//
// Copyright (c) 2021 clib authors
// MIT licensed
//

#include "clib-package-installer.h"
#include "asprintf/asprintf.h"
#include "clib-cache.h"
#include <debug/debug.h>
#include <fs/fs.h>
#include <hash/hash.h>
#include <libgen.h>
#include <logger/logger.h>
#include <mkdirp/mkdirp.h>
#include <path-join/path-join.h>
#include <pthread.h>
#include <registry-manager.h>
#include <repository.h>
#include <string.h>
#include <tempdir/tempdir.h>
#include <limits.h>

CURLSH *clib_package_curl_share;
//TODO, cleanup somewhere curl_share_cleanup(clib_package_curl_share);

static debug_t _debugger;

#define _debug(...)                                \
  ({                                               \
    if (!(_debugger.name))                         \
      debug_init(&_debugger, "package-installer"); \
    debug(&_debugger, __VA_ARGS__);                \
  })

#define E_FORMAT(...)           \
  ({                            \
    rc = asprintf(__VA_ARGS__); \
    if (-1 == rc)               \
      goto cleanup;             \
  });

static hash_t *visited_packages = 0;

#ifdef HAVE_PTHREADS
typedef struct clib_package_lock clib_package_lock_t;
struct clib_package_lock {
  pthread_mutex_t mutex;
};

static clib_package_lock_t lock = {PTHREAD_MUTEX_INITIALIZER};

#endif

static clib_secrets_t secrets = NULL;
static registries_t registries = NULL;

void clib_package_installer_init(registries_t _registries, clib_secrets_t _secrets) {
  secrets = _secrets;
  registries = _registries;
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

    dep = (clib_package_dependency_t *) node->val;
    char *package_id = clib_package_get_id(dep->author, dep->name);
    slug = clib_package_slug(dep->author, dep->name, dep->version);
    if (NULL == slug)
      goto loop_cleanup;

    registry_package_ptr_t package_info = registry_manager_find_package(registries, package_id);
    if (!package_info) {
      debug(&_debugger, "Package %s not found in any registry.", slug);
      return -1;
    }

    pkg = clib_package_new_from_slug_and_url(slug, registry_package_get_href(package_info), verbose);
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

int clib_package_install_executable(clib_package_t *pkg, const char *dir, int verbose) {
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

  clib_package_set_prefix(pkg, path_max);

  const char *configure = pkg->configure;

  if (0 == configure) {
    configure = ":";
  }

  memset(dir_path, 0, path_max);
  realpath(dir, dir_path);

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
    if (-1 == rc)
      goto cleanup;
  }

  if (!package_opts.global && pkg->makefile) {
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
  int max = package_opts.concurrency;
#endif

  if (0 == package_opts.prefix) {
#ifdef HAVE_PTHREADS
    pthread_mutex_lock(&lock.mutex);
#endif
#ifdef _GNU_SOURCE
    char *prefix = secure_getenv("PREFIX");
#else
    char *prefix = getenv("PREFIX");
#endif

    if (prefix) {
      package_opts.prefix = prefix;
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

  if (0 == package_opts.force && pkg && pkg->name) {
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

  clib_package_set_prefix(pkg, path_max);

  if (!(pkg_dir = path_join(dir, pkg->name))) {
    rc = -1;
    goto cleanup;
  }

  if (!package_opts.global) {
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

  if (!package_opts.global && NULL != pkg->src) {
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
  if (!package_opts.global && pkg->makefile) {
    _debug("fetch: %s/%s", pkg->repo, pkg->makefile);
    repository_file_handle_t handle = repository_download_package_file(pkg->url, clib_package_get_id(pkg->author, pkg->name), pkg->version, pkg->makefile, pkg_dir);
    if (handle == NULL) {
      goto cleanup;
    }

#ifdef HAVE_PTHREADS
    repository_file_finish_download(handle);
    repository_file_free(handle);
#endif
  }

  // if no sources are listed, just install
  if (package_opts.global || NULL == pkg->src)
    goto install;

#ifdef HAVE_PTHREADS
  pthread_mutex_lock(&lock.mutex);
#endif

  if (clib_cache_has_package(pkg->author, pkg->name, pkg->version)) {
    if (package_opts.skip_cache) {
      clib_cache_delete_package(pkg->author, pkg->name, pkg->version);
#ifdef HAVE_PTHREADS
      pthread_mutex_unlock(&lock.mutex);
#endif
      goto download;
    }

    if (0 != clib_cache_load_package(pkg->author, pkg->name, pkg->version, pkg_dir)) {
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
  repository_file_handle_t *handles = malloc(pkg->src->len * sizeof(repository_file_handle_t));
  while ((source = list_iterator_next(iterator))) {
    handles[i] = repository_download_package_file(pkg->url, clib_package_get_id(pkg->author, pkg->repo_name), pkg->version, source->val, pkg_dir);

    if (handles[i] == NULL) {
      list_iterator_destroy(iterator);
      iterator = NULL;
      rc = -1;
      goto cleanup;
    }

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
    struct timespec ts = {0, 1000 * 1000 * 10};
    nanosleep(&ts, NULL);
#endif

#ifdef HAVE_PTHREADS
    if (i < 0) {
      i = 0;
    }

    (void) pending++;

    if (i < max) {
      (void) i++;
    } else {
      while (--i >= 0) {
        repository_file_finish_download(handles[i]);
        repository_file_free(handles[i]);
        (void) pending--;

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
        struct timespec ts = {0, 1000 * 1000 * 10};
        nanosleep(&ts, NULL);
#endif
      }
    }
#endif
  }

#ifdef HAVE_PTHREADS
  while (--i >= 0) {
    repository_file_finish_download(handles[i]);

    (void) pending--;
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
