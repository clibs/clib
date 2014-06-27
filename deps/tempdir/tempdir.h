
//
// tempdir.h
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#ifndef TEMPDIR_H
#define TEMPDIR_H 1

/**
 * Get the system's temporary directory using Python's
 * `tempfile.tempdir` algorithm.  Free the result when
 * done.
 *
 * Returns `NULL` on failure.
 */

char *
gettempdir(void);

#endif
