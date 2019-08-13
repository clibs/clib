#!/bin/sh

throw() {
  echo >&2 "$1"
  exit 1
}

export PREFIX=$PWD/tmp/
rm -rf tmp
mkdir -p tmp
mkdir -p tmp/bin
mkdir -p tmp/lib
cd tmp || exit

# see https://github.com/clibs/clib/issues/45
cat > package.json << EOF
{
  "dependencies": {
    "linenoise": "*",
    "stephenmathieson/substr.c": "*",
    "stephenmathieson/emtter.c": "*"
  }
}
EOF

clib install --skip-cache > /dev/null 2>&1

[ $? -eq 1 ] || throw "expecting exit code of 1";

cd - > /dev/null || exit
rm -rf ./tmp
