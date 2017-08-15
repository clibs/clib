
#include "describe/describe.h"
#include "package.h"

int
main() {
  describe("clib_package_dependency_new") {
    it("should return NULL when given bad input") {
      assert(NULL == clib_package_dependency_new("foo/bar", NULL));
      assert(NULL == clib_package_dependency_new(NULL, "foo/bar"));
    }

    it("should return a clib-dependency when given valid input") {
      clib_package_dependency_t *dep = clib_package_dependency_new("foo/bar", "1.2.3");
      assert(dep);
      assert_str_equal("foo", dep->author);
      assert_str_equal("bar", dep->name);
      assert_str_equal("1.2.3", dep->version);
      clib_package_dependency_free(dep);
    }

    it("should transform \"*\" to \"master\"") {
      clib_package_dependency_t *dep = clib_package_dependency_new("foo/bar", "*");
      assert(dep);
      assert_str_equal("master", dep->version);
      clib_package_dependency_free(dep);
    }

    it("should default to \"clibs\" when no repo author is given") {
      clib_package_dependency_t *dep = clib_package_dependency_new("foo", "master");
      assert(dep);
      assert_str_equal("clibs", dep->author);
      assert_str_equal("master", dep->version);
      clib_package_dependency_free(dep);
    }
  }

  return assert_failures();
}
