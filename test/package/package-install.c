
#include "clib-package.h"
#include "describe/describe.h"
#include "fs/fs.h"
#include "rimraf/rimraf.h"
#include <clib-package.h>
#include "registry-manager.h"
#include "repository.h"
#include "clib-package-installer.h"
#include "strdup/strdup.h"

int main() {
  curl_global_init(CURL_GLOBAL_ALL);
  clib_package_set_opts((clib_package_opts_t) {
      .skip_cache = 1,
      .prefix = 0,
      .force = 1,
  });

  registries_t registries = registry_manager_init_registries(NULL, NULL);
  registry_manager_fetch_registries(registries);
  clib_package_installer_init(registries, NULL);
  repository_init(NULL);

  describe("clib_package_install") {
    it("should return -1 when given a bad package") {
      assert(-1 == clib_package_install(NULL, "./deps", 0));
    }

    it("should install the pkg in its own directory") {
      clib_package_t *pkg =
          clib_package_new_from_slug_and_url("stephenmathieson/case.c@0.1.0", "https://github.com/stephenmathison/case.c", 0);
      assert(pkg);
      assert(0 == clib_package_install(pkg, "./test/fixtures/", 0));
      assert(0 == fs_exists("./test/fixtures/"));
      assert(0 == fs_exists("./test/fixtures/case"));
      clib_package_free(pkg);
      rimraf("./test/fixtures/");
    }

    it("should install the package's clib.json or package.json") {
      clib_package_t *pkg =
          clib_package_new_from_slug_and_url("stephenmathieson/case.c@0.1.0", "https://github.com/stephenmathison/case.c", 0);
      assert(pkg);
      assert(0 == clib_package_install(pkg, "./test/fixtures/", 1));
      assert(0 == fs_exists("./test/fixtures/case/package.json") ||
             0 == fs_exists("./test/fixtures/case/clib.json"));
      clib_package_free(pkg);
      rimraf("./test/fixtures/");
    }

    it("should install the package's sources") {
      clib_package_t *pkg =
          clib_package_new_from_slug_and_url("stephenmathieson/case.c@0.1.0", "https://github.com/stephenmathison/case.c", 0);
      assert(pkg);
      assert(0 == clib_package_install(pkg, "./test/fixtures/", 0));
      assert(0 == fs_exists("./test/fixtures/case/case.c"));
      assert(0 == fs_exists("./test/fixtures/case/case.h"));
      clib_package_free(pkg);
      rimraf("./test/fixtures");
    }

    it("should install the package's dependencies") {
      clib_package_t *pkg =
          clib_package_new_from_slug_and_url("stephenmathieson/mkdirp.c@master", "https://github.com/stephenmathieson/mkdirp.c", 0);
      assert(pkg);
      assert(0 == clib_package_install(pkg, "./test/fixtures/", 0));
      assert(0 == fs_exists("./test/fixtures/path-normalize/"));
      assert(0 == fs_exists("./test/fixtures/path-normalize/path-normalize.c"));
      assert(0 == fs_exists("./test/fixtures/path-normalize/path-normalize.h"));
      assert(0 == fs_exists("./test/fixtures/path-normalize/package.json") ||
             0 == fs_exists("./test/fixtures/path-normalize/clib.json"));
      clib_package_free(pkg);
      rimraf("./test/fixtures");
    }

    it("should not install the package's development dependencies") {
      clib_package_t *pkg =
          clib_package_new_from_slug_and_url("stephenmathieson/trim.c@0.0.2", "https://github.com/stephenmathieson/trim.c", 0);
      assert(pkg);
      assert(0 == clib_package_install(pkg, "./test/fixtures/", 0));
      assert(-1 == fs_exists("./test/fixtures/describe/"));
      assert(-1 == fs_exists("./test/fixtures/describe/describe.h"));
      assert(-1 == fs_exists("./test/fixtures/describe/assertion-macros.h"));
      assert(-1 == fs_exists("./test/fixtures/describe/package.json") &&
             -1 == fs_exists("./test/fixtures/describe/clib.json"));
      clib_package_free(pkg);
      rimraf("./test/fixtures");
    }

    /* This test is currently not feasible because clib has too many dependencies with issues. I propose we re-enable this test at the next release.
    it("should install itself") {
      clib_package_t *pkg =
          clib_package_new_from_slug_and_url("clibs/clib@2.7.0", "https://github.com/clibs/clib", 0);
      assert(pkg);
      assert(0 == clib_package_install(pkg, "./test/fixtures/", 0));
      assert(0 == fs_exists("./test/fixtures/"));

      assert(0 == fs_exists("./test/fixtures/clib/"));
      assert(0 == fs_exists("./test/fixtures/clib/clib-package.c"));
      assert(0 == fs_exists("./test/fixtures/clib/clib-package.h"));
      assert(0 == fs_exists("./test/fixtures/clib/package.json") ||
             0 == fs_exists("./test/fixtures/clib/clib.json"));

      assert(0 == fs_exists("./test/fixtures/http-get/"));
      assert(0 == fs_exists("./test/fixtures/http-get/http-get.c"));
      assert(0 == fs_exists("./test/fixtures/http-get/http-get.h"));
      assert(0 == fs_exists("./test/fixtures/http-get/package.json") ||
             0 == fs_exists("./test/fixtures/http-get/clib.json"));

      assert(0 == fs_exists("./test/fixtures/list/"));
      assert(0 == fs_exists("./test/fixtures/list/list.c"));
      assert(0 == fs_exists("./test/fixtures/list/list.h"));
      assert(0 == fs_exists("./test/fixtures/list/package.json") ||
             0 == fs_exists("./test/fixtures/list/clib.json"));

      assert(0 == fs_exists("./test/fixtures/mkdirp/"));
      assert(0 == fs_exists("./test/fixtures/mkdirp/mkdirp.c"));
      assert(0 == fs_exists("./test/fixtures/mkdirp/mkdirp.h"));
      assert(0 == fs_exists("./test/fixtures/mkdirp/package.json") ||
             0 == fs_exists("./test/fixtures/mkdirp/clib.json"));

      assert(0 == fs_exists("./test/fixtures/parson/"));
      assert(0 == fs_exists("./test/fixtures/parson/parson.c"));
      assert(0 == fs_exists("./test/fixtures/parson/parson.h"));
      assert(0 == fs_exists("./test/fixtures/parson/package.json") ||
             0 == fs_exists("./test/fixtures/parson/clib.json"));

      assert(0 == fs_exists("./test/fixtures/path-join/"));
      assert(0 == fs_exists("./test/fixtures/path-join/path-join.c"));
      assert(0 == fs_exists("./test/fixtures/path-join/path-join.h"));
      assert(0 == fs_exists("./test/fixtures/path-join/package.json") ||
             0 == fs_exists("./test/fixtures/path-join/clib.json"));

      assert(0 == fs_exists("./test/fixtures/path-normalize/"));
      assert(0 == fs_exists("./test/fixtures/path-normalize/path-normalize.c"));
      assert(0 == fs_exists("./test/fixtures/path-normalize/path-normalize.h"));
      assert(0 == fs_exists("./test/fixtures/path-normalize/package.json") ||
             0 == fs_exists("./test/fixtures/path-normalize/clib.json"));

      assert(0 == fs_exists("./test/fixtures/str-ends-with/"));
      assert(0 == fs_exists("./test/fixtures/str-ends-with/str-ends-with.c"));
      assert(0 == fs_exists("./test/fixtures/str-ends-with/str-ends-with.h"));
      assert(0 == fs_exists("./test/fixtures/str-ends-with/package.json") ||
             0 == fs_exists("./test/fixtures/str-ends-with/clib.json"));

      assert(0 == fs_exists("./test/fixtures/str-starts-with/"));
      assert(0 ==
             fs_exists("./test/fixtures/str-starts-with/str-starts-with.c"));
      assert(0 ==
             fs_exists("./test/fixtures/str-starts-with/str-starts-with.h"));
      assert(0 == fs_exists("./test/fixtures/str-starts-with/package.json") ||
             0 == fs_exists("./test/fixtures/str-starts-with/clib.json"));

      clib_package_free(pkg);
      rimraf("./test/fixtures");
    } */
  }

  curl_global_cleanup();
  clib_package_cleanup();

  return assert_failures();
}
