#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
test_data_root="$repo_root/test_data"

mkdir -p "$test_data_root/raster" "$test_data_root/vector" "$test_data_root/models"

tool_candidates=(
  "$repo_root/build/dev-linux/bin/generate_test_data"
  "$repo_root/build/release-linux/bin/generate_test_data"
  "$repo_root/build/dev-macos/bin/generate_test_data"
  "$repo_root/build/release-macos/bin/generate_test_data"
)

tool_path=""
for candidate in "${tool_candidates[@]}"; do
  if [[ -x "$candidate" ]]; then
    tool_path="$candidate"
    break
  fi
done

if [[ -z "$tool_path" ]]; then
  echo "Prepared test fixture directories: $test_data_root"
  echo "generate_test_data executable was not found."
  echo "Build it first, for example:"
  echo "  cmake --preset=dev-linux"
  echo "  cmake --build build/dev-linux --target generate_test_data"
  exit 1
fi

(
  cd "$repo_root"
  "$tool_path"
)

model_path="$test_data_root/models/test_seg_model.onnx"
if [[ ! -f "$model_path" ]]; then
  echo "Warning: raster and vector fixtures were generated, but the ONNX test model is still missing: $model_path"
  echo "AI integration tests still require test_seg_model.onnx to be prepared separately."
else
  echo "All test fixtures are ready: $test_data_root"
fi
