
#include "describe/describe.h"
#include "clib-package.h"

int
main() {
  describe("clib_package_parse_version") {
    char *version = NULL;

    it("should return NULL when given a bad slug") {
      assert(NULL == clib_package_parse_version(NULL));
      assert(NULL == clib_package_parse_version(""));
    }

    it("should default to \"master\"") {
      version = clib_package_parse_version("foo");
      assert_str_equal("master", version);
      free(version);

      version = clib_package_parse_version("foo/bar");
      assert_str_equal("master", version);
      free(version);
    }

    it("should transform \"*\" to \"master\"") {
      version = clib_package_parse_version("*");
      assert_str_equal("master", version);
      free(version);

      version = clib_package_parse_version("foo@*");
      assert_str_equal("master", version);
      free(version);

      version = clib_package_parse_version("foo/bar@*");
      assert_str_equal("master", version);
      free(version);
    }

    it("should support \"name\"-style slugs") {
      version = clib_package_parse_version("foo");
      assert_str_equal("master", version);
      free(version);
    }

    it("should support \"name@version\"-style slugs") {
      version = clib_package_parse_version("foo@bar");
      assert_str_equal("bar", version);
      free(version);

      version = clib_package_parse_version("foo@*");
      assert_str_equal("master", version);
      free(version);

      version = clib_package_parse_version("foo@1.2.3");
      assert_str_equal("1.2.3", version);
      free(version);
    }

    it("should support \"author/name@version\"-style slugs") {
      version = clib_package_parse_version("foo/bar@baz");
      assert_str_equal("baz", version);
      free(version);

      version = clib_package_parse_version("foo/bar@*");
      assert_str_equal("master", version);
      free(version);

      version = clib_package_parse_version("foo/bar@1.2.3");
      assert_str_equal("1.2.3", version);
      free(version);
    }

    // this was a bug in parse-repo.c...
    it("should not be affected after the slug is freed") {
      char *slug = malloc(48);
      assert(slug);
      strcpy(slug, "author/name@version");

      version = clib_package_parse_version(slug);

      assert_str_equal("version", version);
      free(slug);

      assert_str_equal("version", version);

      free(version);
    }
  }

  return assert_failures();
}
