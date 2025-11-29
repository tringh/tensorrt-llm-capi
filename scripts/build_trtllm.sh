#!/bin/bash

set -e

cd tensorrt-llm

rm -rf cpp/build

python3 ./scripts/build_wheel.py \
    --clean \
    --cuda_architectures "86-real" \
    --job_count 10 \