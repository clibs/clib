
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
