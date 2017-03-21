#!/bin/bash
DEBDIR="./debianstatic"
BRANCH=master
set -x

rm -rf packagebuild
mkdir -p packagebuild/debian

if [ ! -d "${DEBDIR}" ]; then
    echo "no such debian directory ${DEBDIR}"
    exit 1
fi

git log -n 1 --pretty=format:%h.%ai.%s > commitstring.txt
if [ -z "$DEBFULLNAME" ]; then
	export DEBFULLNAME=`git log -n 1 --pretty=format:%an`
fi

if [ -z "$DEBEMAIL" ]; then
	export DEBEMAIL=`git log -n 1 --pretty=format:%ae`
fi

if [ -z "$DEBBRANCH" ]; then
	export DEBBRANCH=`echo "${BRANCH}" | sed 's/[\/\_]/-/g'`
fi

if [ -z "$DEBPKGVER" ]; then
  export DEBPKGVER=`git log -n 1 --pretty=oneline --abbrev-commit`
fi

if [ -z "$DCHOPTS" ]; then
	export DCHOPTS="-l ${DEBBRANCH} -u low ${DEBPKGVER}"
fi

echo "DEBDIR:       $DEBDIR"
echo "DEBFULLNAME:  $DEBFULLNAME"
echo "DEBEMAIL:     $DEBEMAIL"
echo "DEBBRANCH:    $DEBBRANCH"
echo "DEBPKGVER:    $DEBPKGVER"
echo "DCHOPTS:      $DCHOPTS"


rsync -ar --exclude=packagebuild \
          --exclude=debianstatic . packagebuild
pushd packagebuild
rsync -ar ../${DEBDIR}/ debian/

cat > /tmp/sed.script << EOF
s%{{name}}%${3}%
EOF

find ./debian -type f -iname "*.in" -print0 | while IFS= read -r -d $'\0' file; do
    outFile=$(echo $file | sed -f /tmp/sed.script)
    cat $file | sed -f /tmp/sed.script > ${outFile%.in}
    rm $file
done
rm /tmp/sed.script

dch ${DCHOPTS}
debuild --no-lintian --no-tgz-check -us -uc
popd
