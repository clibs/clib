#!/bin/sh

throw() {
  echo >&2 "$1"
  exit 1
}

rm -rf tmp
mkdir -p tmp
cd tmp || exit

cat > clib.json << EOF
{
  "registries": [
    "https://gitlab.com/api/v4/projects/25447829/repository/files/README.md/raw?ref=master"
  ],
  "dependencies": {
    "nouwaarom/clib-dependency": "0.0.1"
  }
}
EOF

clib install -c -o tmp > /dev/null ||
  throw "expecting exit code of 0";

{ [ -d ./tmp/clib-dependency ] && [ -f ./tmp/clib-dependency/package.json ]; } ||
  throw "failed to install clib-dependency"

cd - > /dev/null || exit
rm -rf ./tmp
