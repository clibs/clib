
#include <string.h>
#include "describe/describe.h"
#include "clib-package.h"

int
main() {
  describe("clib_package_parse_author") {
    char *author = NULL;

    it("should return NULL when given a bad slug") {
      assert(NULL == clib_package_parse_author(NULL));
    }

    it("should return NULL when unable to parse an author") {
      assert(NULL == clib_package_parse_author("/"));
      assert(NULL == clib_package_parse_author("/name"));
      assert(NULL == clib_package_parse_author("/name@version"));
    }

    it("should default to \"clibs\"") {
      author = clib_package_parse_author("foo");
      assert_str_equal("clibs", author);
      free(author);
    }

    it("should support \"author/name\"-style slugs") {
      author = clib_package_parse_author("author/name");
      assert_str_equal("author", author);
      free(author);
    }

    it("should support \"author/name@version\"-slugs slugs") {
      author = clib_package_parse_author("author/name@master");
      assert_str_equal("author", author);
      free(author);
      author = clib_package_parse_author("author/name@*");
      assert_str_equal("author", author);
      free(author);
    }

    // this was a bug in parse-repo.c...
    it("should not be affected after the slug is freed") {
      char *slug = malloc(48);
      assert(slug);
      strcpy(slug, "author/name@version");

      author = clib_package_parse_author(slug);

      assert_str_equal("author", author);
      free(slug);
      assert_str_equal("author", author);
      free(author);
    }
  }

  return assert_failures();
}
