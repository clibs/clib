#!/bin/sh

git clone https://github.com/rustyrussell/ccan

# Output directory
mkdir clibs

cd ccan || exit

make
make tools

# Convert ccan modules into Clib repos
for file in ccan/*; do
	if [ -d "$file" ]; then
		python ../ccan2clib.py "${file#ccan/}" ../clibs
	fi
done
