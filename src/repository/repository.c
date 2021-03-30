//
// repository.c
//
// Copyright (c) 2021 Elbert van de Put
// MIT licensed
//
#include "repository.h"
#include <logger/logger.h>

#include "debug/debug.h"
#include "github-repository.h"
#include "gitlab-repository.h"
#include <clib-package.h>
#include <clib-secrets.h>
#include <fs/fs.h>
#include <libgen.h>
#include <path-join/path-join.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <url/url.h>

static debug_t _debugger;
#define _debug(...)                              \
  ({                                             \
    if (!(_debugger.name))                       \
      debug_init(&_debugger, "clib-repository"); \
    debug(&_debugger, __VA_ARGS__);              \
  })

struct repository_file_t {
  const char *url;
  const char *dir;
  const char *file;
  const char *secret;
  pthread_t thread;
  pthread_attr_t attr;
  void *data;
};

static pthread_mutex_t mutex;

static clib_secrets_t secrets;

static int fetch_package_file(const char *url, const char *dir, const char *file, const char *secret, repository_file_handle_t *thread_data_ptr);

static void curl_lock_callback(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr) {
  pthread_mutex_lock(&mutex);
}

static void curl_unlock_callback(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr) {
  pthread_mutex_unlock(&mutex);
}

static void init_curl_share() {
  if (0 == clib_package_curl_share) {
    pthread_mutex_lock(&mutex);
    clib_package_curl_share = curl_share_init();
    curl_share_setopt(clib_package_curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
    curl_share_setopt(clib_package_curl_share, CURLSHOPT_LOCKFUNC, curl_lock_callback);
    curl_share_setopt(clib_package_curl_share, CURLSHOPT_UNLOCKFUNC, curl_unlock_callback);
    curl_share_setopt(clib_package_curl_share, CURLOPT_NETRC, CURL_NETRC_OPTIONAL);
    pthread_mutex_unlock(&mutex);
  }
}

void repository_init(clib_secrets_t _secrets) {
  init_curl_share();
  secrets = _secrets;
}

static char *repository_create_url_for_file(const char *package_url, const char *package_id, const char *version, const char *file_path, const char *secret) {
  if (strstr(package_url, "github.com") != NULL) {
    return github_repository_get_url_for_file(package_url, package_id, version, file_path, secret);
  } else if (strstr(package_url, "gitlab") != NULL) {
    return gitlab_repository_get_url_for_file(package_url, package_id, version, file_path, secret);
  } else {
    return NULL;
  }
}

http_get_response_t *repository_fetch_package_manifest(const char *package_url, const char *package_id, const char *version, const char* manifest_file) {
  // Check if there is a secret for the requested repository.
  url_data_t *parsed = url_parse(package_url);
  char *secret = clib_secret_find_for_hostname(secrets, parsed->hostname);
  url_free(parsed);
  char *manifest_url = repository_create_url_for_file(package_url, package_id, version, manifest_file, secret);

  http_get_response_t *res;
  if (strstr(package_url, "gitlab") != NULL) {
    char *key = "PRIVATE-TOKEN";
    unsigned int size = strlen(key) + strlen(secret) + 2;
    char *authentication_header = malloc(size);
    snprintf(authentication_header, size, "%s:%s", key, secret);

    res = http_get_shared(manifest_url, clib_package_curl_share, &authentication_header, 1);
  } else {
    res = http_get_shared(manifest_url, clib_package_curl_share, NULL, 0);
  }

  return res;
}

repository_file_handle_t repository_download_package_file(const char *package_url, const char *package_id, const char *version, const char *file_path, const char *destination_path) {
  // Check if there is a secret for the requested repository.
  url_data_t *parsed = url_parse(package_url);
  char *secret = clib_secret_find_for_hostname(secrets, parsed->hostname);
  url_free(parsed);
  char *url = repository_create_url_for_file(package_url, package_id, version, file_path, secret);

  repository_file_handle_t handle;
  fetch_package_file(url, destination_path, file_path, secret, &handle);

  return handle;
}

void repository_file_finish_download(repository_file_handle_t file) {
  void *rc;
  pthread_join(file->thread, &rc);
}

void repository_file_free(repository_file_handle_t file) {
  // TODO, check what else should be freed.
  free(file);
}

static int fetch_package_file_work(const char *url, const char *dir, const char *file, const char *secret) {
  char *path = NULL;
  int saved = 0;
  int rc = 0;

  if (NULL == url) {
    return 1;
  }

  _debug("file URL: %s", url);

  if (!(path = path_join(dir, basename(file)))) {
    rc = 1;
    goto cleanup;
  }

#ifdef HAVE_PTHREADS
  pthread_mutex_lock(&mutex);
#endif

  if (package_opts.force || -1 == fs_exists(path)) {
    _debug("repository", "fetching %s", url);
    fflush(stdout);

#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&mutex);
#endif

    if (strstr(url, "gitlab") != NULL) {
      char *key = "PRIVATE-TOKEN";
      unsigned int size = strlen(key) + strlen(secret) + 2;
      char *authentication_header = malloc(size);
      snprintf(authentication_header, size, "%s:%s", key, secret);

      rc = http_get_file_shared(url, path, clib_package_curl_share, &authentication_header, 1);
    } else {
      rc = http_get_file_shared(url, path, clib_package_curl_share, NULL, 0);
    }
    saved = 1;
  } else {
#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&mutex);
#endif
  }

  if (-1 == rc) {
#ifdef HAVE_PTHREADS
      pthread_mutex_lock(&mutex);
#endif
      logger_error("error", "unable to fetch %s", url);
      fflush(stderr);
      rc = 1;
#ifdef HAVE_PTHREADS
      pthread_mutex_unlock(&mutex);
#endif
      goto cleanup;
  }

  if (saved) {
#ifdef HAVE_PTHREADS
      pthread_mutex_lock(&mutex);
#endif
      _debug("repository", "saved %s", path);
      fflush(stdout);
#ifdef HAVE_PTHREADS
      pthread_mutex_unlock(&mutex);
#endif
  }

cleanup:

  free(path);
  return rc;
}

#ifdef HAVE_PTHREADS
static void *fetch_package_file_thread(void *arg) {
  repository_file_handle_t data = arg;
  int *status = malloc(sizeof(int));
  int rc = fetch_package_file_work(data->url, data->dir, data->file, data->secret);
  *status = rc;
  pthread_exit((void *) status);
}
#endif

static int fetch_package_file(const char *url, const char *dir, const char *file, const char *secret, repository_file_handle_t *thread_data_ptr) {
#ifndef HAVE_PTHREADS
  return fetch_package_file_work(pkg, dir, file, secret);
#else
  repository_file_handle_t fetch = malloc(sizeof(*fetch));
  int rc = 0;

  if (0 == fetch) {
    return -1;
  }

  *thread_data_ptr = 0;

  memset(fetch, 0, sizeof(*fetch));

  fetch->url = url;
  fetch->dir = dir;
  fetch->file = file;
  fetch->secret = secret;

  rc = pthread_attr_init(&fetch->attr);

  if (0 != rc) {
    free(fetch);
    return rc;
  }

  rc = pthread_create(&fetch->thread, NULL, fetch_package_file_thread, fetch);

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

  *thread_data_ptr = fetch;

  return rc;
#endif
}
