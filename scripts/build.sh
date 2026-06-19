#!/bin/bash
# PCL-CGRE 编译脚本
set -euo pipefail

BUILD_TYPE="${1:-Release}"
JOBS="${2:-$(nproc)}"

echo "==> 配置 (${BUILD_TYPE})..."
cmake -B build -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" .

echo ""
echo "==> 编译 (${JOBS} 并行)..."
cmake --build build --parallel "${JOBS}"

echo ""
echo "==> 完成: ./build/pcl-cgre"
