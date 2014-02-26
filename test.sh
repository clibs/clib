#!/bin/bash

TESTS=`find test/* -type f -perm +111`
EXIT_CODE=0

echo -e "\nRunning clib(1) tests\n"

for t in $TESTS; do
  ./$t
  if [ $? -ne 0 ]; then
    echo >&2 "  (✖) $t"
    EXIT_CODE=1
  else
    echo "  (✓) $t"
  fi
done
echo

exit $EXIT_CODE
