#!/bin/sh
root="$(pwd)"
mkdir -p tmp/test-save
mkdir -p tmp/bin
cp test/data/test-save-clib.json tmp/test-save/clib.json

cd tmp/test-save || exit
../../clib-install --prefix "$root/tmp" -c --save stephenmathieson/tabs-to-spaces@1.0.0 >/dev/null
../../clib-install --prefix "$root/tmp" -c -S darthtrevino/str-concat@0.0.2 >/dev/null
../../clib-install --prefix "$root/tmp" -c --save-dev jwerle/fs.c@0.1.1 >/dev/null
../../clib-install --prefix "$root/tmp" -c -D clibs/parson@1.0.2 >/dev/null
cd - >/dev/null || exit

if ! grep --quiet "stephenmathieson/tabs-to-spaces" tmp/test-save/clib.json; then
  echo >&2 "Failed to find stephenmathieson/tabs-to-spaces saved in clib.json"
  exit 1
fi

if ! grep --quiet "darthtrevino/str-concat" tmp/test-save/clib.json; then
  echo >&2 "Failed to find darthtrevino/strconcat saved in clib.json"
  exit 1
fi

if ! grep --quiet "jwerle/fs.c" tmp/test-save/clib.json; then
  echo >&2 "Failed to find jwerle/fs.c saved in clib.json"
  exit 1
fi

if ! grep --quiet "clibs/parson" tmp/test-save/clib.json; then
  echo >&2 "Failed to find clibs/parson saved in clib.json"
  exit 1
fi

rm -rf "$root/deps/tabs-to-spaces"
