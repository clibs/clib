
#include "describe/describe.h"
#include "clib-package.h"

int
main() {
  describe("clib_package_new") {
    char json[] =
      "{"
      "  \"name\": \"foo\","
      "  \"version\": \"1.0.0\","
      "  \"repo\": \"foobar/foo\","
      "  \"license\": \"mit\","
      "  \"description\": \"lots of foo\","
      "  \"src\": ["
      "    \"foo.h\","
      "    \"foo.c\""
      "  ],"
      "  \"dependencies\": {"
      "    \"blah/blah\": \"1.2.3\","
      "    \"bar\": \"*\","
      "    \"abc/def\": \"master\""
      "  }"
      "}";

    it("should return NULL when given broken json") {
      assert(NULL == clib_package_new("{", 0));
    }

    it("should return NULL when given a bad string") {
      assert(NULL == clib_package_new(NULL, 0));
    }

    it("should return a clib_package when given valid json") {
      clib_package_t *pkg = clib_package_new(json, 0);
      assert(pkg);

      assert_str_equal(json, pkg->json);

      assert_str_equal("foo", pkg->name);
      assert_str_equal("foobar", pkg->author);
      assert_str_equal("1.0.0", pkg->version);
      assert_str_equal("foobar/foo", pkg->repo);
      assert_str_equal("mit", pkg->license);
      assert_str_equal("lots of foo", pkg->description);
      assert(NULL == pkg->install);

      assert(2 == pkg->src->len);
      assert_str_equal("foo.h", list_at(pkg->src, 0)->val);
      assert_str_equal("foo.c", list_at(pkg->src, 1)->val);

      assert(3 == pkg->dependencies->len);

      clib_package_dependency_t *dep0 = list_at(pkg->dependencies, 0)->val;
      assert_str_equal("blah", dep0->name);
      assert_str_equal("1.2.3", dep0->version);

      clib_package_dependency_t *dep1 = list_at(pkg->dependencies, 1)->val;
      assert_str_equal("bar", dep1->name);
      assert_str_equal("master", dep1->version);

      clib_package_dependency_t *dep2 = list_at(pkg->dependencies, 2)->val;
      assert_str_equal("def", dep2->name);
      assert_str_equal("master", dep2->version);

      clib_package_free(pkg);
    }

    it("should support missing src") {
      char json[] =
        "{"
        "  \"name\": \"foo\","
        "  \"version\": \"1.0.0\","
        "  \"repo\": \"foobar/foo\","
        "  \"license\": \"mit\","
        "  \"description\": \"lots of foo\""
        "}";

      clib_package_t *pkg = clib_package_new(json, 0);
      assert(pkg);
      clib_package_free(pkg);
    }
  }

  return assert_failures();
}
