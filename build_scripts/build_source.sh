#! /bin/bash
set -eux

#build release
cmake --build $workdir/build/release

#build debug
cmake --build $workdir/build/debug
