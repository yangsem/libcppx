#!/bin/bash

ARCH=$(uname -m)

export LD_LIBRARY_PATH=../../3rd/lib/${ARCH}:../../lib/${ARCH}:${LD_LIBRARY_PATH}

rm -rf core.*

if [ "$1" == "gdb" ]; then
    gdb --args ./cppx_base_unittest.out "$*"
else
    ./cppx_base_unittest.out "$*"
fi
