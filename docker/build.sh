#!/bin/bash
set -e

# Defaults
TRT_VER="${1:-latest}"
CLION_VER="${2:-2025.2.4}"
IMAGE_NAME="htring/trtdev"
TAG="${IMAGE_NAME}:${TRT_VER}"

echo "Building ${TAG} (Base: ${TRT_VER}, CLion: ${CLION_VER})..."

docker build \
  --build-arg TRT_LLM_VERSION="${TRT_VER}" \
  --build-arg CLION_VERSION="${CLION_VER}" \
  -t "${TAG}" \
  .
