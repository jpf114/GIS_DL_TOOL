# 测试数据说明

本目录用于存放仓库集成测试所需的测试夹具。

目录结构：

- `test_data/raster/test_100x100.tif`
- `test_data/vector/test_polygons.shp`
- `test_data/vector/test_points.geojson`
- `test_data/models/test_seg_model.onnx`

测试数据由 CMake 测试系统自动生成：

1. `prepare_gis_test_data` — 调用 `generate_test_data` 工具生成栅格和矢量夹具
2. `prepare_ai_test_model` — 将 `test_data/models/test_seg_model.onnx` 复制到标准位置

运行完整测试：

```
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
```

注意：AI 集成测试需要 `test_seg_model.onnx`，由 CMake 测试夹具自动准备。
