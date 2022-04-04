#!/bin/sh
set -e
trap exit INT

./contrib/makepkg-x86_64-linux.sh
./contrib/makepkg-x86_64-w64-mingw32.static.sh

make dist-gzip
make dist-zip

sha256sum -- linux-pc-x86_64-workifi-*.tar.gz \
    mingw-w64-x86_64-workifi-*.zip \
    workifi-*.tar.gz workifi-*.zip \
    > SHA2-256SUMS

sha512sum -- linux-pc-x86_64-workifi-*.tar.gz \
    mingw-w64-x86_64-workifi-*.zip \
    workifi-*.tar.gz workifi-*.zip \
    > SHA2-512SUMS
