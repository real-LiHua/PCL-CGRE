#!/bin/bash
# PCL-CGRE 编译脚本
set -euo pipefail

BUILD_TYPE="${1:-Release}"
JOBS="${2:-$(nproc)}"

BUILD_DIR="build"
OUTPUT_NAME="pcl-cgre"

if [ "${BUILD_TYPE,,}" = "debug" ]; then
    BUILD_DIR="build-debug"
fi

echo "==> 配置 (${BUILD_TYPE})..."
cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" .

echo ""
echo "==> 编译 (${JOBS} 并行)..."
cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

echo ""
if [ "${BUILD_TYPE,,}" = "debug" ]; then
    mkdir -p build
    cp "${BUILD_DIR}/${OUTPUT_NAME}" build/pcl-cgre_debug
    echo "==> 完成: ./build/pcl-cgre_debug"
else
    echo "==> 完成: ./build/pcl-cgre"
fi
