//
// clib-cache.h
//
// Copyright (c) 2016-2020 clib authors
// MIT licensed
//

#ifndef CLIB_CACHE_H
#define CLIB_CACHE_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/**
 * Internal setup, creates the base cache dir if necessary
 *
 * @param expiration Cache expiration in seconds
 *
 * @return 0 on success, -1 otherwise
 */
int clib_cache_init(time_t expiration);

/**
 * @return The base base dir
 */
const char *clib_cache_dir(void);

/**
 * At this point the package object is not built yet, and can't rely on it
 *
 * @return 0/1 if the package.json is cached
 */
int clib_cache_has_json(char *author, char *name, char *version);

/**
 * @return The content of the cached package.json, or NULL on error, if not
 * found, or expired
 */
char *clib_cache_read_json(char *author, char *name, char *version);

/**
 * @return Number of written bytes, or -1 on error
 */
int clib_cache_save_json(char *author, char *name, char *version,
                         char *content);

/**
 * @return Number of written bytes, or -1 on error
 */
int clib_cache_delete_json(char *author, char *name, char *version);

/**
 * @return 0/1 if the search cache exists
 */
int clib_cache_has_search(void);

/**
 * @return The content of the search cache, NULL on error, if not found, or
 * expired
 */
char *clib_cache_read_search(void);

/**
 * @return Number of written bytes, or -1 on error, or if there is no search
 * cahce
 */
int clib_cache_save_search(char *content);

/**
 * @return 0 on success, -1 otherwise
 */
int clib_cache_delete_search(void);

/**
 * @return 0/1 if the packe is cached
 */
int clib_cache_has_package(char *author, char *name, char *version);

/**
 * @return 0/1 if the cached package modified date is more or less then the
 * given expiration. -1 if the package is not cached
 */
int clib_cache_is_expired_package(char *author, char *name, char *version);

/**
 * @param target_dir Where the cached package should be copied
 *
 * @return 0 on success, -1 on error, if the package is not found in the cache.
 *         If the cached package is expired, it will be deleted, and -2 returned
 */
int clib_cache_load_package(char *author, char *name, char *version,
                            char *target_dir);

/**
 * @param pkg_dir The downloaded package (e.g. ./deps/my_package).
 *                If the package was already cached, it will be deleted first,
 * then saved
 *
 * @return 0 on success, -1 on error
 */
int clib_cache_save_package(char *author, char *name, char *version,
                            char *pkg_dir);

/**
 * @return 0 on success, -1 on error
 */
int clib_cache_delete_package(char *author, char *name, char *version);

#endif
