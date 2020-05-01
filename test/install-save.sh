#!/bin/sh
mkdir -p tmp/test-save
cp test/data/test-save-package.json tmp/test-save/package.json

cd tmp/test-save || exit
../../clib-install -c --save stephenmathieson/tabs-to-spaces@1.0.0 >/dev/null
../../clib-install -c -S darthtrevino/str-concat@0.0.2 >/dev/null
../../clib-install -c --save-dev jwerle/fs.c@0.1.1 >/dev/null
../../clib-install -c -D clibs/parson@1.0.2 >/dev/null
cd - || exit

if ! grep --quiet "stephenmathieson/tabs-to-spaces" tmp/test-save/package.json; then
  echo >&2 "Failed to find stephenmathieson/tabs-to-spaces saved in package.json"
  exit 1
fi

if ! grep --quiet "darthtrevino/str-concat" tmp/test-save/package.json; then
  echo >&2 "Failed to find darthtrevino/strconcat saved in package.json"
  exit 1
fi

if ! grep --quiet "jwerle/fs.c" tmp/test-save/package.json; then
  echo >&2 "Failed to find jwerle/fs.c saved in package.json"
  exit 1
fi

if ! grep --quiet "clibs/parson" tmp/test-save/package.json; then
  echo >&2 "Failed to find clibs/parson saved in package.json"
  exit 1
fi
