#!/bin/sh

throw() {
  echo >&2 "$1"
  exit 1
}

export PREFIX=$PWD/tmp/
rm -rf tmp
mkdir -p tmp
mkdir -p tmp/bin
mkdir -p tmp/lib
cd tmp || exit

clib install --skip-cache -o tmp ms file hash > /dev/null ||
  throw "expecting successful exit code"

[ -d ./tmp/ms ] && [ -f ./tmp/ms/package.json ] ||
  throw "failed to install ms"

[ -d ./tmp/file ] && [ -f ./tmp/file/package.json ] ||
  throw "failed to install file"

[ -d ./tmp/hash ] && [ -f ./tmp/hash/package.json ] ||
  throw "failed to install hash"

rm -rf ./tmp
