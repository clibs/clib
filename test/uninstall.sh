#!/bin/bash
mkdir -p tmp/bin
trap 'rm -rf tmp' EXIT

clib install -c -N stephenmathieson/tabs-to-spaces@1.0.0 -P tmp > /dev/null || {
  echo >&2 "Failed to install stephenmathieson/tabs-to-spaces"
  exit 1
}

clib uninstall stephenmathieson/tabs-to-spaces -P tmp

# ensure the un-installation worked
command -v tmp/bin/t2s >/dev/null 2>&1 && {
  exit 0
}

echo >&2 "Failed to uninstall stephenmathieson/tabs-to-spaces";
exit 1
