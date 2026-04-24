#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "用法: bash scripts/prepare_test_model.sh /path/to/model.onnx"
  exit 1
fi

source_model="$1"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
target_dir="$repo_root/test_data/models"
target_path="$target_dir/test_seg_model.onnx"

if [[ ! -f "$source_model" ]]; then
  echo "未找到源模型文件：$source_model"
  exit 1
fi

if [[ "${source_model##*.}" != "onnx" ]]; then
  echo "源文件不是 .onnx 模型：$source_model"
  exit 1
fi

mkdir -p "$target_dir"
cp "$source_model" "$target_path"

echo "已准备测试模型：$target_path"
