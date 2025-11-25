#!/bin/bash

set -e

cd tensorrt-llm

rm -rf cpp/build

python3 ./scripts/build_wheel.py \
    --clean \
    --cuda_architectures "86-real" \
    --job_count 10 \
    --extra-cmake-vars="-DCMAKE_VERBOSE_MAKEFILE=ON" 2>&1 | tee build.log
