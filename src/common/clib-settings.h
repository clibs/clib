#ifndef CLIB_SETTINGS_H
#define CLIB_SETTINGS_H

// Shared settings
#define CLIB_PACKAGE_CACHE_TIME 30 * 24 * 60 * 60
#define CLIB_SEARCH_CACHE_TIME 1 * 24 * 60 * 60

#ifdef HAVE_PTHREADS
#define MAX_THREADS 12
#endif

extern const char *manifest_names[];

#endif // CLIB_SRC_COMMON_CLIB_SETTINGS_H
