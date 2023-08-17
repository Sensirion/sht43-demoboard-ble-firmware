#! /bin/bash
set -eux
#create build tree
cmake -B $workdir/build/release -S $workdir -DCMAKE_MAKE_PROGRAM=ninja -DCMAKE_BUILD_TYPE=release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja
cmake -B $workdir/build/debug -S $workdir -DCMAKE_MAKE_PROGRAM=ninja -DCMAKE_BUILD_TYPE=debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja

#build release
cmake --build $workdir/build/release

#build debug
cmake --build $workdir/build/debug
