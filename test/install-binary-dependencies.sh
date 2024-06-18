#!/bin/sh

mkdir -p tmp/bin
trap 'rm -rf tmp' EXIT

clib install -c -N stephenmathieson/tabs-to-spaces@1.0.0 -P tmp > /dev/null || {
  echo >&2 "Failed to install stephenmathieson/tabs-to-spaces"
  exit 1
}

command -v tmp/bin/t2s >/dev/null 2>&1 || {
  echo >&2 "Failed to put t2s on path"
  exit 1
}
