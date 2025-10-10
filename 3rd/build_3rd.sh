#!/bin/bash

WORKDIR=$(pwd)

ARCH=$(uname -m)

LIB_PATH=${WORKDIR}/lib/${ARCH}/

function compile_gtest()
{
    echo "==========Compiling googletest=========="

    cd "$WORKDIR"/googletest && \
    mkdir -p build && cd build && \
    cmake -DCMAKE_CXX_STANDARD=11  ../ && \
    make -j10 && \
    echo "==========Compiled googet success=========" && \
    cp -r lib/*.a "${LIB_PATH}"

    if [ ! $? ]; then
        echo "Failed to compile googletest."
    fi
}

function compile_jsoncpp()
{
    echo "==========Compiling jsoncpp=========="
    
    cd "$WORKDIR"/jsoncpp && \
    mkdir -p build && cd build && \
    cmake -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release ../ && \
    make -j10 && \
    echo "==========Compiled jsoncpp success=========" && \
    cp -r lib/*.a "${LIB_PATH}"

    if [ ! $? ]; then
        echo "Failed to compile jsoncpp."
    fi
}

function main()
{
    echo "==========Building 3rd party libraries=========="

    rm -rf "${LIB_PATH}"
    mkdir -p "${LIB_PATH}"

    compile_gtest
    compile_jsoncpp

    echo "==========Built 3rd party libraries over=========="
}

main
