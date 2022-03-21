#!/bin/sh
set -e
trap exit INT

export CFLAGS="-static -s -O3 -D_FORTIFY_SOURCE=2 -fstack-clash-protection \
               -fcf-protection=full -pie -fpie -fpic -fasynchronous-unwind-tables \
               -fstack-protector-strong -Wall -pedantic -std=c99"

export CPPFLAGS="-DCURL_STATICLIB"

DISTDIR=build/workifi/

echo building workifi...
./autogen.sh && ./configure && make

echo building package...
rm -rf $DISTDIR && mkdir -p $DISTDIR
cp workifi $DISTDIR && cp etc/*.json $DISTDIR
mkdir -p $DISTDIR/logs && mkdir -p $DISTDIR/files
echo "test file 01" > $DISTDIR/files/test-file-1
echo "test file 02" > $DISTDIR/files/test-file-2

cp contrib/workifi.bat $DISTDIR
cp workifi.exe $DISTDIR

cd build && zip -r ../mingw-w64-x86_64-workifi.zip workifi/
cd ../ && rm -rf build
