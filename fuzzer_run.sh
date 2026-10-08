#!/bin/bash

# Builds the fuzz targets with clang (libFuzzer, ASan, UBSan) and runs one of them.
#
# Usage: ./fuzzer_run.sh [target] [seconds] [libFuzzer options...]
#
#   ./fuzzer_run.sh                                      # fuzz_packet for 60 seconds
#   ./fuzzer_run.sh packet 600                           # fuzz_packet for 10 minutes
#   ./fuzzer_run.sh packet 600 -fork=4 -ignore_crashes=1 # do not stop at the first crash
#
# Reproduce a crash:
#   ./build/fuzz/fuzz_packet build/fuzz/crashes/packet/crash-<hash>
#
# The compiler is taken from CXX, otherwise the newest clang++ found in PATH is used.

set -e

TARGET=${1:-packet}
SECONDS_TO_RUN=${2:-60}
shift $(( $# < 2 ? $# : 2 ))

ROOT_DIR=$(cd "$(dirname "$0")" && pwd)
BUILD_DIR=$ROOT_DIR/build/fuzz
SEED_DIR=$ROOT_DIR/fuzz/corpus/$TARGET
CORPUS_DIR=$BUILD_DIR/corpus/$TARGET
CRASH_DIR=$BUILD_DIR/crashes/$TARGET

if [ ! -f "$ROOT_DIR/fuzz/fuzz_$TARGET.cc" ] ; then
    echo "Error: unknown fuzz target '$TARGET'"
    exit 1
fi

if [ -z "$CXX" ] ; then
    CXX=$(compgen -c | grep -E '^clang\+\+(-[0-9]+)?$' | sort -t- -k2 -n | tail -n 1)
fi
if [ -z "$CXX" ] || ! command -v "$CXX" > /dev/null ; then
    echo "Error: clang++ is not found, install clang or set CXX"
    exit 1
fi

mkdir -p "$BUILD_DIR" "$CORPUS_DIR" "$CRASH_DIR"

cmake -S "$ROOT_DIR/fuzz" -B "$BUILD_DIR" -DCMAKE_CXX_COMPILER="$CXX"
cmake --build "$BUILD_DIR" --target "fuzz_$TARGET" -j"$(nproc || echo 2)"

# New inputs go to the first directory, the seeds kept in git are only read.
# 1600 is the size of Buffer, a longer datagram never reaches the parser.
"$BUILD_DIR/fuzz_$TARGET" \
    -max_total_time="$SECONDS_TO_RUN" \
    -max_len=1600 \
    -artifact_prefix="$CRASH_DIR/" \
    "$@" \
    "$CORPUS_DIR" "$SEED_DIR"
