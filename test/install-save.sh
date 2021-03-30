#!/bin/sh
mkdir -p tmp/test-save
cp test/data/test-save-package.json tmp/test-save/package.json

cd tmp/test-save || exit
../../clib-install -c --save clibs/buffer@0.4.0 >/dev/null
../../clib-install -c -S clibs/strdup@0.1.0 >/dev/null
../../clib-install -c --save-dev jwerle/fs.c@0.1.1 >/dev/null
../../clib-install -c -D clibs/list@0.2.0 >/dev/null
cd - >/dev/null || exit

if ! grep --quiet "clibs/buffer" tmp/test-save/package.json; then
  echo >&2 "Failed to find clibs/buffer.json"
  exit 1
fi

if ! grep --quiet "clibs/strdup" tmp/test-save/package.json; then
  echo >&2 "Failed to find clibs/strdup saved in package.json"
  exit 1
fi

if ! grep --quiet "jwerle/fs.c" tmp/test-save/package.json; then
  echo >&2 "Failed to find jwerle/fs.c saved in package.json"
  exit 1
fi

if ! grep --quiet "clibs/list" tmp/test-save/package.json; then
  echo >&2 "Failed to find clibs/list saved in package.json"
  exit 1
fi
