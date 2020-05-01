#!/bin/sh

throw() {
  echo >&2 "$1"
  exit 1
}

clib install -c -o tmp ms file hash > /dev/null ||
  throw "expecting successful exit code"

[ -d ./tmp/ms ] && [ -f ./tmp/ms/package.json ] ||
  throw "failed to install ms"

[ -d ./tmp/file ] && [ -f ./tmp/file/package.json ] ||
  throw "failed to install file"

[ -d ./tmp/hash ] && [ -f ./tmp/hash/package.json ] ||
  throw "failed to install hash"

rm -rf ./tmp
