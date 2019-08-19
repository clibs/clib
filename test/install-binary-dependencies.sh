#!/bin/sh

export PREFIX=$PWD/tmp/
rm -rf tmp
mkdir -p tmp
mkdir -p tmp/bin
mkdir -p tmp/lib
cd tmp || exit

clib install --skip-cache stephenmathieson/tabs-to-spaces@1.0.0 > /dev/null || {
  echo >&2 "Failed to install stephenmathieson/tabs-to-spaces"
  exit 1
}
command -v $PREFIX/bin/t2s >/dev/null 2>&1 || {
  echo >&2 "Failed to put t2s on path"
  exit 1
}
