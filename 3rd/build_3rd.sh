#!/bin/bash

WORKDIR=$(pwd)

ARCH=$(uname -m)

LIB_PATH=${WORKDIR}/lib/${ARCH}/

mkdir -p "${LIB_PATH}"

function compile_gtest()
{
    echo "Compiling googletest ..."

    cd "$WORKDIR"/googletest && \
    mkdir -p build && cd build && \
    cmake -DCMAKE_CXX_STANDARD=11  ../ && \
    make -j10 && \
    echo "Compiled googletest successfully." && \
    cp -r lib/*.a "${LIB_PATH}"

    if [ ! $? ]; then
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
