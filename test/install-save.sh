#!/bin/bash
mkdir -p tmp/test-save
cp test/data/test-save-package.json tmp/test-save/package.json

pushd tmp/test-save
../../clib-install --save stephenmathieson/tabs-to-spaces@1.0.0 > /dev/null
popd

if grep --quiet stephenmathieson/tabs-to-spaces tmp/test-save/package.json; then
  echo >&2 "Failed to find dependency saved in package.json"
  exit 1
fi
