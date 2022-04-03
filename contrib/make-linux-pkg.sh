#!/bin/sh
set -e
trap exit INT

export CFLAGS="-s -O3 -pipe -D_FORTIFY_SOURCE=2 -fstack-clash-protection \
               -fcf-protection=full -pie -fpie -fpic -fasynchronous-unwind-tables \
               -fstack-protector-strong -Wall -pedantic -std=c99"

export LDFLAGS="-Wl,-z,defs -Wl,-z,now -Wl,-z,relro"

DISTDIR=build/workifi/

echo building workifi...
autoreconf --install
./configure 
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
tar cvfz ../linux-pc-x86_64-workifi.tar.gz workifi/
cd ../ 
rm -rf build
