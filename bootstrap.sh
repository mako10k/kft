#!/bin/bash

set -e

# ------------------------------
# VARIABLES
# ------------------------------
basedir="$(dirname "$0")"
owner_expect="$(id -nu):$(id -ng)"

# ------------------------------
# RESET ENVIRONMENT
# ------------------------------
if [ -f "$basedir/Makefile" ]; then
    make distclean || true
fi

# ------------------------------
# REPAIRE OWNER
# ------------------------------
find "$basedir" | while read f; do
    owner_actual="$(stat -c'%U:%G' "$f")"
    if [ "$owner_expect" != "$owner_actual" ]; then
        sudo chown "$owner_expect" "$f"
    fi
done

# ------------------------------
# CHANGE COMPILER
# ------------------------------
if [ -z "${CC}" ]; then
    if clang -v > /dev/null 2>&1; then
        CC=clang
    else
        CC=gcc
    fi
fi

# ------------------------------
# ENABLE command output
# ------------------------------
set -x

# ------------------------------
# BUILD SUBMODULES
# ------------------------------
git submodule update --init --recursive
git submodule foreach ./bootstrap.sh

# ------------------------------
# GENERATE configure
# ------------------------------
autoreconf -fiv

# ------------------------------
# CONFIGURE 'Makefile's
# ------------------------------
./configure CC="$CC"

# ------------------------------
# BUILD
# ------------------------------
make all

# ------------------------------
# TEST
# ------------------------------
make check

# ------------------------------
# INSTALL
# ------------------------------
sudo make install
