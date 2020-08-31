#!/bin/sh

mkdir -p tmp/bin
TESTS=$(find test/* -type f -perm -111)
EXIT_CODE=0
export PATH="$PWD:$PATH"

printf "\nRunning clib(1) tests\n\n"

for t in $TESTS; do
  if ! ./"$t"; then
    echo >&2 "  (✖) $t"
    EXIT_CODE=1
  else
    echo "  (✓) $t"
  fi
done
echo


printf "\nRunning clib package tests\n\n"
cd test/package && make clean

if ! make test; then
    EXIT_CODE=1
fi

cd ../../

printf "\nRunning clib cache tests\n\n"
cd test/cache 

if ! make test; then
    EXIT_CODE=1
fi

cd ../../

exit $EXIT_CODE
