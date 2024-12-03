#!/bin/bash

export CC=clang

set -ex

autoreconf --force --install --verbose
./configure
make
sudo make install
