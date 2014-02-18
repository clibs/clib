#ifndef FS_H
#define FS_H 1


/**
 * fs.h
 *
 * copyright 2013 - joseph werle <joseph.werle@gmail.com>
 */


#include <stdio.h>
#include <sys/stat.h>

#ifdef _WIN32
#define FS_OPEN_READ "rb"
#define FS_OPEN_WRITE "wb"
#define FS_OPEN_READWRITE "rwb"
#else
#define FS_OPEN_READ "r"
#define FS_OPEN_WRITE "w"
#define FS_OPEN_READWRITE "rw"
#endif

typedef struct stat fs_stats;



/**
 * Prints the last error to stderr
 */

void
fs_error (const char *prefix);


/**
 * Opens a file with given flags
 * and returns a file descriptor
 */

FILE *
fs_open (const char *path, const char *flags);


/**
 * Closes a given file descriptor
 */

int
fs_close (FILE *file);


/**
 * Moves a path to a new destination
 */

int
fs_rename (const char *from, const char *to);


/**
 * Stats a given path and returns
 * a `struct stat`
 */

fs_stats *
fs_stat (const char *path);


/**
 * Stats a given file descriptor
 */

fs_stats *
fs_fstat (FILE *file);


/**
 * Stats a given link path
 */

fs_stats *
fs_lstat (const char *path);


/**
 * Truncates a file to a specified
 * length from a given file descriptor
 */

int
fs_ftruncate (FILE *file, int len);


/**
 * Truncates a file to a specified
 * len from a given file path
 */

int
fs_truncate (const char *path, int len);


/**
 * Changes ownership of a given
 * file path to a given owner
 * and group
 */

int
fs_chown (const char *path, int uid, int gid);


/**
 * Change ownership of a given
 * file descriptor to a given owner
 * and group
 */

int
fs_fchown (FILE *file, int uid, int gid);


/**
 * Returns the size of a file from
 * a given file path
 */

size_t
fs_size (const char *path);


/**
 * Returns the size of a file
 * from a given file descriptor
 */

size_t
fs_fsize (FILE *file);


/**
 * Change ownership of a given
 * link path to a given owner
 * and group
 */

int
fs_lchown (const char *path, int uid, int gid);


/**
 * Reads a file by a given file
 * path
 */

char *
fs_read (const char *path);


/**
 * Reads a file by a given
 * file path by a given `n`
 * number of bytes
 */

char *
fs_nread (const char *path, int len);


/**
 * Reads a file by a given
 * file descriptor
 */

char *
fs_fread (FILE *file);


/**
 * Reads a file by a given
 * file descriptor by a given
 * `n` number of bytes
 */

char *
fs_fnread (FILE *file, int len);


/**
 * Writes a buffer
 * to a given file path
 */

int
fs_write (const char *path, const char *buffer);


/**
 * Writes `n` bytes of a buffer to a given
 * file path
 */

int
fs_nwrite (const char *path, const char *buffer, int len);


/**
 * Writes a buffer to a given
 * file stream
 */

int
fs_fwrite (FILE *file, const char *buffer);


/**
 * Writes `n` bytes of a buffer
 * to a given file stream
 */

int
fs_fnwrite (FILE *file, const char *buffer, int len);


/**
 * Makes a directory and returns 0
 * on success or -1 on failure
 */

int
fs_mkdir (const char *path, int mode);


/**
 * Removes a directory and returns
 * 0 on success and -1 on failure
 */

int
fs_rmdir (const char *path);

/**
 * Check if the given `path` exists
 */

int
fs_exists (const char *path);


#endif
