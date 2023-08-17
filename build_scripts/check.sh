#! /bin/bash

set -eux
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

# spell checker
echo "spell checker"
cd source
codespell -I $workdir/ignoreWords.txt
cd $workdir
