#!/bin/bash

export CC=clang
user=$(id -nu)
group=$(id -ng)

set -ex

sudo chown -R $user:$group "$(dirname "$0")"
autoreconf -fiv
./configure
make clean all
sudo make install
