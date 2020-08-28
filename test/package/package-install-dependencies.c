
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

  describe("clib_package_install_dependencies") {
    it("should return -1 when given a bad package") {
      assert(-1 == clib_package_install_dependencies(NULL, "./deps", 0));
    }

    it("should install the dep in its own directory") {
      clib_package_t *dep = clib_package_new_from_slug("stephenmathieson/mkdirp.c", 0);
      assert(dep);
      assert(0 == clib_package_install_dependencies(dep, "./test/fixtures/", 0));
      assert(0 == fs_exists("./test/fixtures/"));
      assert(0 == fs_exists("./test/fixtures/path-normalize"));
      clib_package_free(dep);
      rimraf("./test/fixtures/");
    }

    it("should install the dependency's package.json") {
      clib_package_t *pkg = clib_package_new_from_slug("stephenmathieson/mkdirp.c", 0);
      assert(pkg);
      assert(0 == clib_package_install_dependencies(pkg, "./test/fixtures/", 0));
      assert(0 == fs_exists("./test/fixtures/path-normalize/package.json"));
      clib_package_free(pkg);
      rimraf("./test/fixtures/");
    }

    it("should install the dependency's sources") {
      clib_package_t *pkg = clib_package_new_from_slug("stephenmathieson/mkdirp.c", 0);
      assert(pkg);
      assert(0 == clib_package_install_dependencies(pkg, "./test/fixtures/", 0));
      assert(0 == fs_exists("./test/fixtures/path-normalize/path-normalize.c"));
      assert(0 == fs_exists("./test/fixtures/path-normalize/path-normalize.h"));
      clib_package_free(pkg);
      rimraf("./test/fixtures");
    }

    it("should install the dependency's dependencies") {
      clib_package_t *pkg = clib_package_new_from_slug("stephenmathieson/rimraf.c", 0);
      assert(pkg);
      assert(0 == clib_package_install_dependencies(pkg, "./test/fixtures/", 0));
      // deps
      assert(0 == fs_exists("./test/fixtures/path-join/"));
      assert(0 == fs_exists("./test/fixtures/path-join/package.json"));
      assert(0 == fs_exists("./test/fixtures/path-join/path-join.c"));
      assert(0 == fs_exists("./test/fixtures/path-join/path-join.h"));
      // deps' deps
      assert(0 == fs_exists("./test/fixtures/str-ends-with/"));
      assert(0 == fs_exists("./test/fixtures/str-ends-with/package.json"));
      assert(0 == fs_exists("./test/fixtures/str-ends-with/str-ends-with.h"));
      assert(0 == fs_exists("./test/fixtures/str-ends-with/str-ends-with.c"));
      assert(0 == fs_exists("./test/fixtures/str-starts-with/"));
      assert(0 == fs_exists("./test/fixtures/str-starts-with/package.json"));
      assert(0 == fs_exists("./test/fixtures/str-starts-with/str-starts-with.h"));
      assert(0 == fs_exists("./test/fixtures/str-starts-with/str-starts-with.c"));
      clib_package_free(pkg);
      rimraf("./test/fixtures");
    }

    it("should handle unresolved packages") {
      char json[] =
        "{"
        "  \"dependencies\": {"
        "    \"linenoise\": \"*\","
        "    \"stephenmathieson/substr.c\": \"*\","
        "    \"stephenmathieson/emtter.c\": \"*\""
        "  }"
        "}";

      clib_package_t *pkg = clib_package_new(json, 0);
      assert(pkg);

      int r = clib_package_install_dependencies(pkg, "./test/fixtures/", 0);
      // should fail
      assert(-1 == r);

      assert(0 == fs_exists("./test/fixtures/linenoise/package.json"));
      assert(0 == fs_exists("./test/fixtures/substr/package.json"));
      assert(-1 == fs_exists("./test/fixtures/emtter/"));

      clib_package_free(pkg);
      rimraf("./test/fixtures");
    }
  }

  curl_global_cleanup();
  clib_package_cleanup();

  return assert_failures();
}
