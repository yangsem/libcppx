#!/bin/bash

ARCH=$(uname -m)

export LD_LIBRARY_PATH=../../3rd/lib/${ARCH}:${LD_LIBRARY_PATH}

./cppx_base_unittest.out "$*"