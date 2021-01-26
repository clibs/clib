
//
// http-get.h
//
// Copyright (c) 2013 Stephen Mathieson
// MIT licensed
//


#ifndef HTTP_GET_H
#define HTTP_GET_H 1

#include <stdlib.h>

#define HTTP_GET_VERSION "0.4.0"

typedef struct {
  char *data;
  size_t size;
  long status;
  int ok;
} http_get_response_t;

http_get_response_t *http_get(const char *);
http_get_response_t *http_get_shared(const char *, void *);

int http_get_file(const char *, const char *);
int http_get_file_shared(const char *, const char *, void *);

void http_get_free(http_get_response_t *);

#endif
