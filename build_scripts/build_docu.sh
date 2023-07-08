#! /bin/bash
set -eux
#
cd $workdir
doxygen ./documentation/doxygen/Doxyfile
cd ..
