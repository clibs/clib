#include <stdio.h>
#include <unistd.h>
#include <mkdirp/mkdirp.h>
#include "rimraf/rimraf.h"
#include "fs/fs.h"
#include "copy/copy.h"
#include "package.h"
#include "cache.h"


#define GET_PKG_CACHE(pkg) char pkg_cache[BUFSIZ]; \
                      package_cache_path(pkg_cache, pkg);

#define GET_JSON_CACHE(a, n, v) char json_cache[BUFSIZ]; \
                  json_cache_path(json_cache, a, n, v);

#ifdef _WIN32
#define BASE_DIR getenv("AppData")
#else
#define BASE_DIR getenv("HOME")
#endif


#define BASE_CACHE_PATTERN "%s/.cache/clib"
#define PKG_CACHE_PATTERN "%s/%s_%s_%s"
#define JSON_CACHE_PATTERN "%s/%s_%s_%s.json"

/** Portable PATH_MAX ? */
static char package_cache_dir[BUFSIZ];
static char search_cache[BUFSIZ];
static char json_cache_dir[BUFSIZ];
static time_t expiration;


static void
json_cache_path(char *pkg_cache, char *author, char *name, char *version)
{
    sprintf(pkg_cache, JSON_CACHE_PATTERN, json_cache_dir, author, name, version);
}

static void
package_cache_path(char *json_cache, clib_package_t *pkg)
{
    sprintf(json_cache, PKG_CACHE_PATTERN, package_cache_dir, pkg->author, pkg->name, pkg->version);
}

const char *
clib_cache_dir(void)
{
    return package_cache_dir;
}

const char *
clib_cache_search_file(void)
{
    return search_cache;
}

static int
check_dir(char *dir)
{
    if (0 != (fs_exists(dir))) {
        return mkdirp(dir, 0700);
    }
    return 0;
}

int 
clib_cache_init(time_t exp)
{
    expiration = exp;

    sprintf(package_cache_dir, BASE_CACHE_PATTERN"/packages", BASE_DIR);
    sprintf(json_cache_dir, BASE_CACHE_PATTERN"/json", BASE_DIR);
    sprintf(search_cache, BASE_CACHE_PATTERN"/search.html", BASE_DIR);

    if (0 != check_dir(package_cache_dir)) {
        return -1;
    }
    if (0 != check_dir(json_cache_dir)) {
        return -1;
    }

    return 0;
}

static int 
is_expired(char *cache)
{
    fs_stats *stat = fs_stat(cache);

    if (!stat) {
        return -1;
    }

    time_t modified = stat->st_mtime;
    time_t now = time(NULL);
    free(stat);

    return now - modified >= expiration;
}

int 
clib_cache_has_json(char *author, char *name, char *version)
{
    GET_JSON_CACHE(author, name, version);

    return 0 == fs_exists(json_cache) && !is_expired(json_cache);
}

char *
clib_cache_read_json(char *author, char *name, char *version)
{
    GET_JSON_CACHE(author, name, version);

    if (is_expired(json_cache)) {
        return NULL;
    }

    return fs_read(json_cache);
}

int 
clib_cache_save_json(char *author, char *name, char *version, char *content)
{
    GET_JSON_CACHE(author, name, version);

    return fs_write(json_cache, content);
}

int 
clib_cache_delete_json(char *author, char *name, char *version)
{
    GET_JSON_CACHE(author, name, version);

    return unlink(json_cache);
}

int 
clib_cache_has_search(void)
{
    return 0 == fs_exists(search_cache);
}

char *
clib_cache_read_search(void)
{
    if (!clib_cache_has_search()) {
        return NULL;
    }
    return fs_read(search_cache);
}

int 
clib_cache_save_search(char *content)
{
    return fs_write(search_cache, content);
}

int 
clib_cache_delete_search(void)
{
    return unlink(search_cache);
}

int 
clib_cache_has_package(clib_package_t *pkg)
{
    GET_PKG_CACHE(pkg);

    return 0 == fs_exists(pkg_cache) && !is_expired(pkg_cache);
}

int 
clib_cache_is_expired_package(clib_package_t *pkg)
{
    GET_PKG_CACHE(pkg);

    return is_expired(pkg_cache);
}

int 
clib_cache_save_package(clib_package_t *pkg, char *pkg_dir)
{
    GET_PKG_CACHE(pkg);

    if (0 == fs_exists(pkg_cache)) {
        rimraf(pkg_cache);
    }

    return copy_dir(pkg_dir, pkg_cache);
}

int 
clib_cache_load_package(clib_package_t *pkg, char *target_dir)
{
    GET_PKG_CACHE(pkg);

    if (-1 == fs_exists(pkg_cache)) {
        return -1;
    }

    if (is_expired(pkg_cache)) {
        rimraf(pkg_cache);

        return -2;
    }

    return copy_dir(pkg_cache, target_dir);
}

int 
clib_cache_delete_package(clib_package_t *pkg)
{
    GET_PKG_CACHE(pkg);

    return rimraf(pkg_cache);
}
