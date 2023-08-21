#! /bin/bash

set -eux

#create build tree; we need to do this here since it's required by clang tidy
cmake -B $workdir/build/release -S $workdir -DCMAKE_MAKE_PROGRAM=ninja -DCMAKE_BUILD_TYPE=release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja
cmake -B $workdir/build/debug -S $workdir -DCMAKE_MAKE_PROGRAM=ninja -DCMAKE_BUILD_TYPE=debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja

cd $workdir
#editor config checker
echo "editor config checker"
editorconfig-checker

# source code formatting with clang-format
echo "clang-format"
find source -type f -iregex ".*\.\(c\|h\)" | xargs clang-format --dry-run --Werror -style=file

# cpp-check
echo "cpp-check"
cppcheck --std=c11 --platform=unix32 -q --error-exitcode=1 -DPLACE_IN_SECTION --language=c ./source

echo "clang-tidy"
run-clang-tidy -p $workdir/build/release -extra-arg="-DSTATIC_CODE_ANALYSIS=1" -extra-arg="--std=gnu99" $workdir/source


# spell checker
echo "spell checker"
cd source
codespell -I $workdir/ignoreWords.txt
cd $workdir
