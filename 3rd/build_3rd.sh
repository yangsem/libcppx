#!/bin/bash

WORKDIR=`pwd`

ARCH=$(uname -m)

mkdir -p lib/${ARCH}/

function compile_gtest()
{
    echo "Compiling googletest ..."

    cd $WORKDIR/googletest && \
    mkdir -p build && cd build && \
    cmake -DCMAKE_CXX_STANDARD=11  ../ && \
    make -j10 && \
    echo "Compiled googletest successfully." && \
    cp -r lib/*.a $WORKDIR/lib/${ARCH}/

    if [ $? -ne 0 ]; then
        echo "Failed to compile googletest."
    fi
}

function main()
{
    echo "Building 3rd party libraries ..."

    compile_gtest

    echo "Built 3rd party libraries over ..."
}

main
