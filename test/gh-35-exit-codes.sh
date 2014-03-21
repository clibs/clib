#!/bin/bash

# see #35

clib install stephenmathieson/rot132.c > /dev/null 2>&1
[ $? -ne 0 ] || {
  echo >&2 "Failed installations should exit with a non-zero exit code";
  exit 1
}
