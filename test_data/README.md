# 测试数据说明

本目录用于存放仓库集成测试所需的测试夹具。

## 目录结构

### CMake 自动生成的测试夹具

- `test_data/raster/test_100x100.tif`
- `test_data/vector/test_polygons.shp`
- `test_data/vector/test_points.geojson`
- `test_data/models/test_seg_model.onnx`

### 真实遥感测试数据

- `test_data/real/test_remote_sensing_1024x1024.tif` — 1024x1024 模拟遥感影像（含建筑物、道路、水体、植被，EPSG:32650）

## 测试数据生成

### CMake 测试系统

1. `prepare_gis_test_data` — 调用 `generate_test_data` 工具生成栅格和矢量夹具
2. `prepare_ai_test_model` — 将 `test_data/models/test_seg_model.onnx` 复制到标准位置

### 真实遥感测试数据

```bash
conda activate GIS
python tools/generate_test_image.py
```

## 运行完整测试

```
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
```

## 端到端分割测试

```bash
# 使用 U-Net 模型
gis_ai_cli segment models/unet_building/unet_building_512x512.onnx test_data/real/test_remote_sensing_1024x1024.tif output.tif output.shp --tile-size 512 --stride 384 --blend gaussian --verbose

# 使用 SegFormer 模型
gis_ai_cli segment models/segformer_landcover/segformer_b0_landcover_512x512.onnx test_data/real/test_remote_sensing_1024x1024.tif output.tif output.shp --tile-size 512 --stride 384 --blend gaussian --verbose
```

注意：AI 集成测试需要 `test_seg_model.onnx`，由 CMake 测试夹具自动准备。
