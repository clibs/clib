2.2.0 / 2020-08-25
==================
  * Integrateing the clib-package into the main repo

1.8.1 / 2017-08-21
==================

  * travis: add @clibbot token
  * Upload Windows binaries to tagged Github Releases (#147)
  * Pre-compiled Windows binaries for future Github releases (#143)

1.8.0 / 2016-11-02
==================

  * Fix version number
  * Updated broken link to article (#134)
  * Add ccan to clib conversion script (#128)
  * Add 'curl-config' to Makefile to find cURL (#117)
  * Update .gitignore

1.7.0 / 2016-01-17
==================

  * test: fix installed `parson` version
  * deps,package: update `parson`
  * package,deps: update `logger`
  * deps,package: update clib-package (adds ability to install from repo in "package.json [@willemt])
  * Makefile: rename `MKDIR_P` to `MKDIRP`

1.6.0 / 2016-01-17
==================

  * test: remove kgabis/parson comment
  * deps,package: update `parson`
  * deps,package: update `str-replace`
  * install: add `--save` and `--save-dev` options (#124, @darthtrevino)

1.5.0 / 2015-11-17
==================

  * clib-search: add `-c/--skip-cache` flag for ignoring $TMPDIR/clib-search.cache
  * test.sh cygwin compatibility
  * add cygwin compatible

1.4.2 / 2015-01-06
==================

  * clib-install: Use `gzip -dc` for Windows support (#103, @mattn)

1.4.1 / 2014-12-15
==================

  * install: Use name from repo instead of package name when installing (#67, @jb55)

1.4.0 / 2014-12-01
==================

  * src: Add `debug()`s
  * deps: Update clib-package and asprintf
  * deps: Fix whitespace in case
  * deps: Update parson

1.3.0 / 2014-12-01
==================

  * test: Add checks for clib-search --no-color
  * clib-search: Rename option `--nocolor` to `--no-color`
  * clib-search: Add --nocolor option to clib-search for uncolorized terminal output [@breckinloggins, #100]

1.2.4 / 2014-11-21
==================

  * package: Update stephenmathieson/case.c to 0.1.3
  * travis: add apt-get update before installing
  * deps: Update parson
  * History: typo

1.2.3 / 2014-10-24
==================

  * package: Update deps
  * Bug #52: Deletion of unnecessary checks before calls of the function "free"
  * Readme: add a blog article
  * Fix copyright statement across project

1.2.2 / 2014-07-03
==================

 * test: Fix unescaped backticks
 * deps: Update tempdir
 * package.json: Update tempdir.c for a Windows bugfix
 * test: Add two basic testcases for clib-search(1)
 * clib: Use strdup rather than str-copy
 * clib-search: Use strdup rather than str-copy
 * package.json: Update deps to remove str-copy usage

1.2.1 / 2014-06-28
==================

 * clib-search: Use `gettempdir()` for computing the system's temp path
 * clib-install: Use `gettempdir()` for computing the system's temp path
 * package.json: Add stephenmathieson/tempdir.c to dependencies
 * package.json: Pin asprintf.c at 0.0.2
 * clib.c: Use littlstar/asprintf.c
 * clib-install.c: Use littlstar/asprintf.c
 * package.json: Add littlstar/asprintf for `asprintf` on Windows
 * package.json: Update clib-package to 0.2.6
 * Check $TEMP before using /tmp
 * package.json: Update stephenmathieson/clib-package and jwerle/fs.c
 * Readme: Added link to package.json explanation

1.2.0 / 2014-05-23
==================

 * package: Update clib-package to 0.2.4
 * Change GitHub endpoint (raw.github.com -> raw.githubusercontent.com)
 * install: Fetch makefiles
 * clib: Fix bad `free`
 * clib: Fix possible overflow in subcommand handling
 * install: Fix possible memory overflows
 * Improve Readme syntax highlighting

1.1.6 / 2014-05-16
==================

 * pkg: Update clib-package to 0.2.3 (closes #63)

1.1.5 / 2014-05-06
==================

 * README: Use SVG badge for Travis
 * Fix quiet flag typo
 * Fix unused variable warning on non-WIN32 platforms
 * Add a test for brace expansion

1.1.4 / 2014-03-26
==================

 * Fix Windows build

1.1.3 / 2014-03-26
==================

 * Windows port of Makefile
 * Surpress writes to stderr in tests
 * Use clibs/logger
 * Update clib-package to 0.2.2
 * Readme: Add Travis status button.
 * Add an explicit LICENSE file

1.1.2 / 2014-03-17
==================

 * Update clib-package to 0.2.1
 * Fix leaks in `help` and `search` commands

1.1.1 / 2014-03-10
==================

 * Update clib-package
 * Remove npmignore

1.1.0 / 2014-03-10
==================

 * Update console-colors
 * Added unset `__STRICT_ANSI__` for MinGW
 * Test installing multiple libs from the clibs namespace
 * Test cleanup when done
 * Add a test for installing multiple libs
 * Update str-flatten for memory fixes
 * Update case.c
 * search: Update wiki-registry
 * Update usage and examples
 * Update which
 * Add help command

1.0.1 / 2014-02-26
==================

 * Fix installation of executables with dependencies
 * Add note about homebrew
 * Add Travis and a primitive testing framework
 * Account for out of range return codes
 * Compile objects

1.0.0 / 2014-02-24
==================

 * Pin deps
 * Update clib-package
 * Use console-colors instead of using escape sequence directly
 * Update which for str-copy usage
 * Update wiki-registry for parser fix and str_copy usage
 * Update str-flatten for memory fixes
 * Update mkdirp for Windows support
 * Update all dependencies
 * search: handle str_copy failure
 * Use str_copy instead of strdup
 * Fix package.json
 * Add note about libcurl
 * fix search output padding
 * suppress unused function warnings
 * Add 'install' key to package.json
 * Free wiki package
 * Add clib-search
 * Update installation instructions
 * Initial port to C
 * remove node stuff

0.2.0 / 2013-12-19
==================

 * add support for omitting "clibs/" on install
 * change: install packages to their own subdirectory

0.1.0 / 2013-10-11
==================

 * add search command

0.0.3 / 2012-11-20
==================

  * add cpm-install(1)
  * add -o, --out <dir>
  * change default dir to ./deps

0.0.2 / 2012-10-30
==================

  * add bin

0.0.1 / 2010-01-03
==================

  * Initial release
