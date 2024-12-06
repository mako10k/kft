#!/bin/bash

basedir="$(dirname "$0")"
user=$(id -nu)
group=$(id -ng)

set -ex

sudo chown -R $user:$group "$basedir"

if [ -z "${CC}" ]; then
    if clang -v > /dev/null 2>&1; then
        CC=clang
    else
        CC=gcc
    fi
fi

if [ -f "$basedir/Makefile" ]; then
    make distclean || true
fi

autoreconf -fiv
./configure CC="$CC" --enable-debug
make clean all check
sudo make install
