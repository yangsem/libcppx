#!/bin/bash

set -e

lcov --capture --directory ../../base/build --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_report

# 打开 coverage_report/index.html 查看覆盖率报告
xdg-open coverage_report/index.html