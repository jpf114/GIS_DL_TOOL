# 测试数据说明

本目录用于存放仓库集成测试所需的测试夹具。

当前约定的目录结构如下：

- `test_data/raster/test_100x100.tif`
- `test_data/vector/test_polygons.shp`
- `test_data/vector/test_points.geojson`
- `test_data/models/test_seg_model.onnx`

可通过以下脚本准备目录结构：

- Windows：`powershell -ExecutionPolicy Bypass -File scripts/generate_test_data.ps1`
- Linux/macOS：`bash scripts/generate_test_data.sh`

当前脚本会先创建标准目录，并提示下一步如何扩展为真实夹具生成流程。后续如果引入自动生成逻辑，应继续复用这两个脚本作为统一入口。
