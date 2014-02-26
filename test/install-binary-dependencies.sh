#!/bin/bash

clib install stephenmathieson/tabs-to-spaces@1.0.0 > /dev/null
[ $? -eq 0 ] || {
  echo >&2 "Failed to install stephenmathieson/tabs-to-spaces";
  exit 1
}
