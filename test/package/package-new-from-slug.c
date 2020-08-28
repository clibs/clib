
#include "describe/describe.h"
#include "clib-package.h"

int
main() {
  curl_global_init(CURL_GLOBAL_ALL);

  describe("clib_package_new_from_slug") {
    it("should return NULL when given a bad slug") {
      assert(NULL == clib_package_new_from_slug(NULL, 0));
    }

    it("should return NULL when given a slug missing a name") {
      assert(NULL == clib_package_new_from_slug("author/@version", 0));
    }

    it("should return NULL when given slug which doesn't resolve") {
      assert(NULL == clib_package_new_from_slug("abc11234", 0));
    }

    it("should build the correct package") {
      clib_package_t *pkg = clib_package_new_from_slug("stephenmathieson/case.c@0.1.0", 0);
      assert(pkg);
      assert_str_equal("case", pkg->name);
      assert_str_equal("0.1.0", pkg->version);
      assert_str_equal("stephenmathieson/case.c", pkg->repo);
      assert_str_equal("MIT", pkg->license);
      assert_str_equal("String case conversion utility", pkg->description);
      clib_package_free(pkg);
    }

    it("should force package version numbers") {
      clib_package_t *pkg = clib_package_new_from_slug("stephenmathieson/mkdirp.c@0.0.1", 0);
      assert(pkg);
      assert_str_equal("0.0.1", pkg->version);
      clib_package_free(pkg);
    }

    it("should use package version if version not provided") {
      clib_package_t *pkg = clib_package_new_from_slug("stephenmathieson/mkdirp.c", 0);
      assert(pkg);
      assert_str_equal("0.1.5", pkg->version);
      clib_package_free(pkg);
    }

    it("should save the package's json") {
      clib_package_t *pkg = clib_package_new_from_slug("stephenmathieson/str-replace.c@8ca90fb", 0);
      assert(pkg);
      assert(pkg->json);

      char expected[] =
        "{\n"
        "  \"name\": \"str-replace\",\n"
        "  \"version\": \"0.0.3\",\n"
        "  \"repo\": \"stephenmathieson/str-replace.c\",\n"
        "  \"description\": \"String replacement in C\",\n"
        "  \"keywords\": [ \"string\", \"replace\" ],\n"
        "  \"license\": \"MIT\",\n"
        "  \"src\": [\n"
        "    \"src/str-replace.c\",\n"
        "    \"src/str-replace.h\",\n"
        "    \"deps/occurrences.c\",\n"
        "    \"deps/occurrences.h\"\n"
        "  ]\n"
        "}\n";

      assert_str_equal(expected, pkg->json);
      clib_package_free(pkg);
    }
  }

  return assert_failures();
}
