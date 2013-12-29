
//
// http-get.c
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include "http-get.h"

/**
 * HTTP GET write callback
 */

static size_t http_get_cb(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  http_get_response_t *res = userp;

  res->data = realloc(res->data, res->size + realsize + 1);
  if (NULL == res->data) {
    fprintf(stderr, "not enough memory!");
    return 0;
  }

  memcpy(&(res->data[res->size]), contents, realsize);
  res->size += realsize;
  res->data[res->size] = 0;

  return realsize;
}

/**
 * Perform an HTTP(S) GET on `url`
 */

http_get_response_t *http_get(const char *url) {
  CURL *req = curl_easy_init();

  static http_get_response_t res;
  res.data = malloc(1);
  res.size = 0;

  curl_easy_setopt(req, CURLOPT_URL, url);
  curl_easy_setopt(req, CURLOPT_HTTPGET, 1);
  curl_easy_setopt(req, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(req, CURLOPT_WRITEFUNCTION, http_get_cb);
  curl_easy_setopt(req, CURLOPT_WRITEDATA, (void *)&res);

  int c = curl_easy_perform(req);

  curl_easy_getinfo(req, CURLINFO_RESPONSE_CODE, &res.status);
  res.ok = (200 == res.status && CURLE_ABORTED_BY_CALLBACK != c) ? 1 : 0;
  curl_easy_cleanup(req);

  return &res;
}

/**
 * HTTP GET file write callback
 */

static size_t http_get_file_cb(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}

/**
 * Request `url` and save to `file`
 */

int http_get_file(const char *url, const char *file) {
  CURL *req = curl_easy_init();
  if (!req) return -1;

  FILE *fp = fopen(file, "wb");
  if (!fp) return -1;

  curl_easy_setopt(req, CURLOPT_URL, url);
  curl_easy_setopt(req, CURLOPT_HTTPGET, 1);
  curl_easy_setopt(req, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(req, CURLOPT_WRITEFUNCTION, http_get_file_cb);
  curl_easy_setopt(req, CURLOPT_WRITEDATA, fp);
  int res = curl_easy_perform(req);

  long status;
  curl_easy_getinfo(req, CURLINFO_RESPONSE_CODE, &status);

  curl_easy_cleanup(req);
  fclose(fp);

  return (200 == status && CURLE_ABORTED_BY_CALLBACK != res) ? 0 : -1;
}

/**
 * Free the given `res`
 */

void http_get_free(http_get_response_t *res) {
  if (NULL == res) return;
  if (res->data) free(res->data);
}
