#!/bin/sh

clib help 2> /dev/null
[ $? -eq 1 ] || {
  echo >&2 "Expected \`clib help\` to fail"
  exit 1
}

ACTUAL=$(clib help install)
EXPECTED=$(clib install --help)

[ "$ACTUAL" = "$EXPECTED" ] || {
  echo >&2 "\`help install\` should output clib-install --help"
  exit 1
}

ACTUAL=$(clib help search)
EXPECTED=$(clib search --help)

[ "$ACTUAL" = "$EXPECTED" ] || {
  echo >&2 "\`help search\` should output clib-search --help"
  exit 1
}

