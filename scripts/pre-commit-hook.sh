#!/usr/bin/env bash

function format_and_restage_file {
  local file="$1"
  # Ensure the file exists, otherwise clang-format gets very angry.
  if [ -f "$file" ]; then
    clang-format -i -style=LLVM "$file"
    git add "$file"
  fi
}

if ! command -v clang-format &> /dev/null; then
  echo "Unable to format staged changes: clang-format not found."
  exit
fi

# Format each staged file (ending in `.c` or `.h`).
for file in `git diff-index --cached --name-only HEAD | grep -iE '\.(c|h)$' ` ; do
  format_and_restage_file "$file"
done
