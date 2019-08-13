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

clib install --skip-cache -o tmp stephenmathieson/trim.c stephenmathieson/case.c > /dev/null ||
  throw "expecting successful exit code"

[ -d ./tmp/case ] && [ -f ./tmp/case/package.json ] ||
  throw "failed to install case.c"

[ -d ./tmp/trim ] && [ -f ./tmp/trim/package.json ] ||
  throw "failed to install trim.c"

rm -rf ./tmp
