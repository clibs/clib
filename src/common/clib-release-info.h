//
// clib-release-info.h
//
// Copyright (c) 2016-2021 clib authors
// MIT licensed
//

#ifndef CLIB_RELEASE_INFO_H
#define CLIB_RELEASE_INFO_H

/**
 * @return NULL on failure, char * otherwise that must be freed
 */
const char *clib_release_get_latest_tag(void);

#endif
