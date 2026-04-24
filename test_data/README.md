# 测试数据说明

本目录用于存放仓库集成测试所需的测试夹具。

当前约定的目录结构如下：

- `test_data/raster/test_100x100.tif`
- `test_data/vector/test_polygons.shp`
- `test_data/vector/test_points.geojson`
- `test_data/models/test_seg_model.onnx`

可通过以下脚本准备测试夹具：

- Windows：`powershell -ExecutionPolicy Bypass -File scripts/generate_test_data.ps1`
- Linux/macOS：`bash scripts/generate_test_data.sh`

当前脚本会优先调用仓库中已构建的 `generate_test_data` 工具，自动生成栅格和矢量夹具。

需要注意：

- `test_data/raster/test_100x100.tif` 与矢量夹具可以通过现有工具生成
- `test_data/models/test_seg_model.onnx` 目前还没有仓库内自动生成逻辑
- 如果缺少 `test_seg_model.onnx`，AI 集成测试会继续跳过或失败前置检查

如果你本地已经有可用的 ONNX 测试模型，可以通过以下脚本放入标准位置：

- Windows：`powershell -ExecutionPolicy Bypass -File scripts/prepare_test_model.ps1 -SourceModel <模型路径>`
- Linux/macOS：`bash scripts/prepare_test_model.sh <模型路径>`

因此，当前仓库已经具备“自动生成栅格/矢量夹具 + 规范导入 ONNX 测试模型”的能力。
