#!/bin/bash

ARCH=$(uname -m)

export LD_LIBRARY_PATH=../../3rd/lib/${ARCH}:../../lib/${ARCH}:${LD_LIBRARY_PATH}

rm -rf core.*

if [ "$1" == "gdb" ]; then
    if [ -z "$2" ]; then
        gdb --args ./cppx_base_unittest.out
    else
        gdb --args ./cppx_base_unittest.out --gtest_filter="$2"
    fi  
elif [ "$1" == "valgrind" ]; then
    if [ -z "$2" ]; then
        valgrind --tool=memcheck --leak-check=yes --undef-value-errors=yes --track-origins=yes ./cppx_base_unittest.out
    else
        valgrind --tool=memcheck --leak-check=yes --undef-value-errors=yes --track-origins=yes ./cppx_base_unittest.out --gtest_filter="$2"
    fi
else
    if [ -z "$1" ]; then
        ./cppx_base_unittest.out
    else
        ./cppx_base_unittest.out --gtest_filter="$1"
    fi
fi
