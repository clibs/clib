#!/bin/bash

./clib-install -c stephenmathieson/tabs-to-spaces@1.0.0 > /dev/null || {
  echo >&2 "Failed to install stephenmathieson/tabs-to-spaces"
  exit 1
}

./clib-uninstall stephenmathieson/tabs-to-spaces

# ensure the un-installation worked
command -v t2s >/dev/null 2>&1 && {
  exit 0
}

echo >&2 "Failed to uninstall stephenmathieson/tabs-to-spaces";
exit 1
