
#!/bin/bash

./clib-install stephenmathieson/tabs-to-spaces

# ensure the installation worked
command -v t2s >/dev/null 2>&1 || {
  echo >&2 "Failed to install stephenmathieson/tabs-to-spaces";
  exit 1
}

./clib-uninstall stephenmathieson/tabs-to-spaces

# ensure the un-installation worked
command -v t2s >/dev/null 2>&1 && {
  exit 0
}

echo >&2 "Failed to uninstall stephenmathieson/tabs-to-spaces";
exit 1
