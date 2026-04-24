#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
test_data_root="$repo_root/test_data"

mkdir -p "$test_data_root/raster" "$test_data_root/vector" "$test_data_root/models"

echo "已准备测试夹具目录：$test_data_root"
echo "下一步请扩展此脚本，调用仓库工具生成栅格、矢量和 ONNX 测试夹具。"
