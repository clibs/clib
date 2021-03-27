#ifndef CLIB_SRC_CLIB_SETTINGS_H
#define CLIB_SRC_CLIB_SETTINGS_H

// Shared settings
#define CLIB_PACKAGE_CACHE_TIME 30 * 24 * 60 * 60

#ifdef HAVE_PTHREADS
#define MAX_THREADS 12
#endif

const char *manifest_names[] = {"clib.json", "package.json", 0};

#endif//CLIB_SRC_CLIB_SETTINGS_H
