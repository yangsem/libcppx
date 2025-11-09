#!/bin/bash

set -e

# 清理后重新编译

if [ "$1" == "clean" ] && [ -d "build" ]; then
    cmake --build build --target clean
    rm -rf build
    exit 0
fi

cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build