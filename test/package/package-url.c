#include "clib-cache.h"
#include "clib-package.h"
#include "describe/describe.h"
#include "rimraf/rimraf.h"

int main() {
  clib_cache_init(100);
  rimraf(clib_cache_dir());

  describe("clib_package_url") {
    it("should return NULL when given a bad author") {
      assert(NULL == clib_package_url(NULL, "name", "version"));
    }

    it("should return NULL when given a bad name") {
      assert(NULL == clib_package_url("author", NULL, "version"));
    }

    it("should return NULL when given a bad version") {
      assert(NULL == clib_package_url("author", "name", NULL));
    }

    it("should build a GitHub url") {
      char *url = clib_package_url("author", "name", "version");
      assert_str_equal("https://raw.githubusercontent.com/author/name/version",
                       url);
      free(url);
    }
  }

  return assert_failures();
}
