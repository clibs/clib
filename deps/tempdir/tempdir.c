
//
// tempdir.c
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "strdup/strdup.h"
#include "tempdir.h"

// 1. The directory named by the TMPDIR environment variable.
// 2. The directory named by the TEMP environment variable.
// 3. The directory named by the TMP environment variable.
// 4. A platform-specific location:
//   4.1 On RiscOS, the directory named by the Wimp$ScrapDir
//       environment variable.
//   4.2 On Windows, the directories C:\TEMP, C:\TMP, \TEMP,
//       and \TMP, in that order.
//   4.3 On all other platforms, the directories /tmp,
//       /var/tmp, and /usr/tmp, in that order.
// 5. As a last resort, the current working directory.

static const char *env_vars[] = {
  "TMPDIR",
  "TEMP",
  "TMP",
  // RiscOS (4.1)
  "Wimp$ScrapDir",
  NULL,
};


#ifdef _WIN32
  // 4.2
  static const char *platform_dirs[] = {
    "C:\\TEMP",
    "C:\\TMP",
    "\\TEMP",
    "\\TMP",
    NULL,
  };
#else
  // 4.3
  static const char *platform_dirs[] = {
    "/tmp",
    "/var/tmp",
    "/usr/tmp",
    NULL,
  };
#endif

/**
 * Check if the file at `path` exists and is a directory.
 *
 * Returns `0` if both checks pass, and `-1` if either fail.
 */

static int
is_directory(const char *path) {
  struct stat s;
  if (-1 == stat(path, &s)) return -1;
  return 1 == S_ISDIR(s.st_mode) ? 0 : -1;
}

char *
gettempdir(void) {
  // check ENV vars (1, 2, 3)
  for (int i = 0; env_vars[i]; i++) {
    char *dir = getenv(env_vars[i]);
    if (dir && 0 == is_directory(dir)) {
      return strdup(dir);
    }
  }

  // platform-specific checks (4)
  for (int i = 0; platform_dirs[i]; i++) {
    if (0 == is_directory(platform_dirs[i])) {
      return strdup(platform_dirs[i]);
    }
  }

  // fallback to cwd (5)
  char cwd[256];
  if (NULL != getcwd(cwd, sizeof(cwd))) {
    return strdup(cwd);
  }

  return NULL;
}
