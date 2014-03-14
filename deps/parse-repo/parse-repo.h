
//
// parse-repo.h
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#ifndef PARSE_REPO_H
#define PARSE_REPO_H 1

/**
 * Parse the repo owner from the given slug.  If
 * no owner is provided and `fallback != NULL`,
 * will return a copy of `fallback`.
 *
 * Free the result when you're done with it.
 */

char *
parse_repo_owner(const char *, const char *);

/**
 * Parse the repo name from the given slug.
 *
 * Free the result when you're done with it.
 */

char *
parse_repo_name(const char *);

/**
 * Parse the repo version from the given slug.  If
 * no version is present and `fallback != NULL`,
 * will return a copy of `fallback`.
 *
 * Free the result when you're done with it.
 */

char *
parse_repo_version(const char *, const char *);

#endif
