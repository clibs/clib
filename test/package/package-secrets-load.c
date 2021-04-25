#include "describe/describe.h"
#include <stdio.h>
#include "clib-secrets.h"

int main() {
  describe("clib_secrets") {
    it("should load secrets from a valid json.") {
      clib_secrets_t secrets = clib_secrets_load_from_file("clib_secrets.json");
      assert(secrets != NULL);
    }

    it("should provide secrets for a domain.") {
      clib_secrets_t secrets = clib_secrets_load_from_file("clib_secrets.json");
      char *github_secret = clib_secret_find_for_hostname(secrets, "github.com");
      assert(strcmp(github_secret, "GithubSecret") == 0);
      char *gitlab_secret = clib_secret_find_for_hostname(secrets, "gitlab.com");
      assert(strcmp(gitlab_secret, "GitlabSecret") == 0);
    }

    return assert_failures();
  }
}
