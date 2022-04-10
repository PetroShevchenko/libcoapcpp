#!/bin/sh

set -evu

export GTEST_DIR=$(pwd)/third-party/googletest/googletest
export GMOCK_DIR=$(pwd)/third-party/googletest/googlemock

mkdir -p build
cd build && cmake .. && make -j$(nproc || echo 2)
mkdir -p examples && cd examples && cmake ../../examples/POSIX && make -j$(nproc || echo 2) install
