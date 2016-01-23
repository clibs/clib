#!/bin/bash

git clone https://github.com/rustyrussell/ccan

# Output directory
mkdir clibs

cd ccan

make
make tools

# Convert ccan modules into Clib repos
for file in $(ls ccan); do
	if [[ -d ccan/$file ]]; then
		python ../ccan2clib.py $file ../clibs
	fi
done
