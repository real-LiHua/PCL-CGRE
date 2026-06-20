#!/bin/bash
set -euo pipefail

BUILD_TYPE="${1:-Release}"
BUILD_NAME="pcl-cgre${BUILD_TYPE,,}"
JOBS="${2:-$(nproc)}"

echo "==> 配置 (${BUILD_TYPE})..."
cmake -B build -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" .

echo ""
echo "==> 编译 (${JOBS} 并行)..."
cmake --build build --parallel "${JOBS}"

echo ""
echo "==> 完成: build/${BUILD_NAME}"
