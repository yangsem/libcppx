#!/bin/bash

if [ "$1" == "base" ]; then
    cd ../ && make clean && make -j6
    cd -
else
    make clean && make -j6
fi