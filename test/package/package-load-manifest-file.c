#include "describe/describe.h"
#include "clib-package.h"

int
main() {
  describe("clib_package_load_local_manifest") {
    it("should load a local package if available") {
      int verbose = 0;
      clib_package_t* pkg = clib_package_load_local_manifest(verbose);
      assert(NULL != pkg);
      if (pkg) {
        clib_package_free(pkg);
      }
    }
  }

  return assert_failures();
}
