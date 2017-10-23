#!/bin/sh

CACHE=$TMPDIR/clib-search.cache
rm -f "$CACHE" 2> /dev/null

N=$(clib search | wc -l)
# lame check for more than 100 lines of output
[ "$N" -lt 100 ] && {
  echo >&2 "Expected \`clib search\` to return at least 100 results"
  exit 1
}

TRIM=$(clib search trim)
case "$TRIM" in
  *"stephenmathieson/trim.c"*)
    :
    ;;
  *)
    echo >&2 "Expected \`clib search trim\` to output stephenmathieson/trim.c"
    exit 1
    ;;
esac
