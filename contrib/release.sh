#!/bin/sh
set -e
trap exit INT

grep -q 'AC_INIT(\[workifi\]' configure.ac || (echo && \
    echo "  Cannot find Workifi's configure.ac file," && \
    echo "  Please run this script from the root folder" && \
    echo "  of Workifi's source" && echo && exit 1)

./contrib/makepkg-x86_64-pc-linux-gnu.sh
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
