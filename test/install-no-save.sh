#!/bin/sh
mkdir -p tmp/test-save/bin
RUNDIR="$PWD"
trap 'rm -rf "$RUNDIR/tmp"' EXIT

cp test/data/test-save-package.json tmp/test-save/package.json

cd tmp/test-save || exit
../../clib-install -c --no-save stephenmathieson/tabs-to-spaces@1.0.0 -P . >/dev/null
../../clib-install -c -N darthtrevino/str-concat@0.0.2 >/dev/null
../../clib-install -c --dev --no-save jwerle/fs.c@0.1.1 >/dev/null
../../clib-install -c -d --no-save clibs/parson@1.0.2 >/dev/null
cd - || exit

if grep --quiet "stephenmathieson/tabs-to-spaces" tmp/test-save/package.json; then
  echo >&2 "Found stephenmathieson/tabs-to-spaces saved in package.json but --no-save was used"
  exit 1
fi

if grep --quiet "darthtrevino/str-concat" tmp/test-save/package.json; then
  echo >&2 "Found darthtrevino/strconcat saved in package.json but --no-save was used"
  exit 1
fi

if grep --quiet "jwerle/fs.c" tmp/test-save/package.json; then
  echo >&2 "Found jwerle/fs.c saved in package.json but --no-save was used"
  exit 1
fi

if grep --quiet "clibs/parson" tmp/test-save/package.json; then
  echo >&2 "Found clibs/parson saved in package.json but --no-save was used"
  exit 1
fi
