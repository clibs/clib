
#include "describe/describe.h"
#include "rimraf/rimraf.h"
#include "fs/fs.h"
#include "clib-package.h"

int
main() {
  curl_global_init(CURL_GLOBAL_ALL);
  clib_package_set_opts((clib_package_opts_t) {
    .skip_cache = 1,
    .prefix = 0,
    .force = 1,
  });

  describe("clib_package_install_development") {
    it("should return -1 when given a bad package") {
      assert(-1 == clib_package_install_development(NULL, "./deps", 0));
    }

    it("should return -1 when given a bad dir") {
      clib_package_t *pkg = clib_package_new_from_slug("stephenmathieson/mkdirp.c", 0);
      assert(pkg);
      assert(-1 == clib_package_install_development(pkg, NULL, 0));
      clib_package_free(pkg);
    }

    it("should install the package's development dependencies") {
      clib_package_t *pkg = clib_package_new_from_slug("stephenmathieson/trim.c@0.0.2", 0);
      assert(pkg);
      assert(0 == clib_package_install_development(pkg, "./test/fixtures", 0));
      assert(0 == fs_exists("./test/fixtures/describe"));
      assert(0 == fs_exists("./test/fixtures/describe/describe.h"));
      assert(0 == fs_exists("./test/fixtures/describe/package.json"));
      assert(0 == fs_exists(
        "./test/fixtures/assertion-macros/assertion-macros.h"
      ));
      assert(0 == fs_exists("./test/fixtures/assertion-macros/package.json"));
      rimraf("./test/fixtures");
      clib_package_free(pkg);
    }
  }

  curl_global_cleanup();
  clib_package_cleanup();

  return assert_failures();
}
