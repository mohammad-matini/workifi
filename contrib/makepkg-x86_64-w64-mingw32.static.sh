#!/bin/sh

set -e
trap exit INT

# We use mxe to setup a mingw cross-compilation environment
HOST=x86_64-w64-mingw32.static

PKG_CONFIG="$(which ${HOST}-pkg-config)"
AR="$(which ${HOST}-ar)"
LD="$(which ${HOST}-ld)"
CC="$(which ${HOST}-gcc)"

CFLAGS="-s -O3 -pipe -D_FORTIFY_SOURCE=2 -fstack-clash-protection \
        -fcf-protection=full -pie -fpie -fpic -fasynchronous-unwind-tables \
        -fstack-protector-strong -Wall -pedantic -std=c99"

export HOST PKG_CONFIG AR LD CC CFLAGS

DISTDIR=build/workifi/

VERSION=$(git tag --points-at HEAD | sed 's/^v//')
if [ -z "$VERSION" ]; then
    VERSION=$(git rev-parse --short HEAD)
fi

echo building workifi...
autoreconf --install
./configure --host="$HOST"
make clean && make

echo building package...
rm -rf $DISTDIR
mkdir -p $DISTDIR
mkdir -p $DISTDIR/logs
mkdir -p $DISTDIR/files

echo "test file 01" > $DISTDIR/files/test-file-1
echo "test file 02" > $DISTDIR/files/test-file-2

cp workifi.exe $DISTDIR
cp etc/*.json $DISTDIR
cp contrib/workifi.bat $DISTDIR

cd build
zip -r "../mingw-w64-x86_64-workifi-$VERSION.zip" workifi/
cd ../
rm -rf build
