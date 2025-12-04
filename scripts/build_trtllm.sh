#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/../tensorrt-llm"

echo "LFS pulling..."
git lfs install && git lfs pull

python3 scripts/build_wheel.py \
    --generator "Ninja" \
    --cuda_architectures "86-real" \
    --job_count $(( $(nproc) / 2 )) \
    --build_type Release \
    --use_ccache \
    --install