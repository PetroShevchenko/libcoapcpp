#!/bin/sh

set -evu

#git submodule init
#git submodule update

mkdir -p build
cd build && cmake .. && make
