#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <describe/describe.h>
#include "rimraf/rimraf.h"
#include "fs/fs.h"
#include "../../src/common/clib-cache.h"


#define assert_exists(f) assert_equal(0, fs_exists(f));

static void assert_cached_dir(char *pkg_dir, int exists)
{
    assert_equal(exists, fs_exists(pkg_dir));
}

static void assert_cached_file(char *pkg_dir, char *file)
{
    char path[BUFSIZ];
    sprintf(path, "%s/%s", pkg_dir, file);

    assert_exists(path);
}

static void assert_cached_files(char *pkg_dir)
{
    assert_cached_file(pkg_dir, "copy.c");
    assert_cached_file(pkg_dir, "copy.h");
    assert_cached_file(pkg_dir, "package.json");
}

int
main() {

  rimraf(clib_cache_dir());

  describe("clib-cache opearions") {


    char *author = "author";
    char *name = "pkg";
    char *version = "1.2.0";
    char pkg_dir[BUFSIZ];


    it("should initialize succesfully") {
      assert_equal(0, clib_cache_init(1));
    }

    sprintf(pkg_dir, "%s/author_pkg_1.2.0", clib_cache_dir());

    it("should manage package the cache") {
      assert_equal(0, clib_cache_save_package(author, name, version, "../../deps/copy"));
      assert_equal(1, clib_cache_has_package(author, name, version));
      assert_equal(0, clib_cache_is_expired_package(author, name, version));

      assert_cached_dir(pkg_dir, 0);
      assert_cached_files(pkg_dir);
    }

    it("should manage the json cache") {
      char *cached_json;

      assert_equal(0, clib_cache_has_json("a", "n", "v"));
      assert_null(clib_cache_read_json("a", "n", "v"));

      assert_equal(2, clib_cache_save_json("a", "n", "v", "{}"));
      assert_equal(0, strcmp("{}", cached_json = clib_cache_read_json("a", "n", "v")));
      free(cached_json);
      assert_equal(1, clib_cache_has_json("a", "n", "v"));

      assert_equal(0, clib_cache_delete_json("a", "n", "v"));
      assert_equal(0, clib_cache_has_json("a", "n", "v"));
      assert_null(clib_cache_read_json("a", "n", "v"));
    }

    it("should manage the search cache") {
      char *cached_search;

  clib_cache_delete_search();

      assert_equal(0, clib_cache_has_search());
      assert_null(clib_cache_read_search());

      assert_equal(13, clib_cache_save_search("<html></html>"));
      assert_equal(1, clib_cache_has_search());
      assert_equal(0, strcmp("<html></html>", cached_search = clib_cache_read_search()));
      free(cached_search);

      assert_equal(0, clib_cache_delete_search());
      assert_equal(0, clib_cache_has_search());
      assert_null(clib_cache_read_search());
    }
  }

  return assert_failures();
}
