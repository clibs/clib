#!/bin/sh

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

exit $EXIT_CODE
