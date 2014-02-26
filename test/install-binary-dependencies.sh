#!/bin/bash

clib install stephenmathieson/tabs-to-spaces@1.0.0 > /dev/null
[ $? -eq 0 ] || {
  echo >&2 "Failed to install stephenmathieson/tabs-to-spaces";
  exit 1
}
command -v t2s >/dev/null 2>&1 || {
  echo >&2 "Failed to put t2s on path";
  exit 1
}
