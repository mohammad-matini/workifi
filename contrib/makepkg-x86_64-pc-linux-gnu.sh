#!/bin/sh
set -e
trap exit INT

grep -q 'AC_INIT(\[workifi\]' configure.ac || (echo && \
    echo "  Cannot find Workifi's configure.ac file," && \
    echo "  Please run this script from the root folder" && \
    echo "  of Workifi's source" && echo && exit 1)

CFLAGS="-s -O3 -pipe -D_FORTIFY_SOURCE=2 -fstack-clash-protection \
        -fcf-protection=full -pie -fpie -fpic -fasynchronous-unwind-tables \
        -fstack-protector-strong -Wall -pedantic -std=c99"

LDFLAGS="-Wl,-z,defs -Wl,-z,now -Wl,-z,relro"

export CFLAGS LDFLAGS

DISTDIR=build/workifi/

VERSION=$(git tag --points-at HEAD | sed 's/^v//')
if [ -z "$VERSION" ]; then
    VERSION=$(git rev-parse --short HEAD)
fi

echo building workifi...
autoreconf --install
./configure
make clean
make

echo building package...
rm -rf $DISTDIR
mkdir -p $DISTDIR
mkdir -p $DISTDIR/logs
mkdir -p $DISTDIR/files

echo "test file 01" > $DISTDIR/files/test-file-1
echo "test file 02" > $DISTDIR/files/test-file-2

cp workifi $DISTDIR
cp etc/*.json $DISTDIR

cd build
tar cvfz "../linux-pc-x86_64-workifi-$VERSION.tar.gz" workifi/
cd ../
rm -rf build
