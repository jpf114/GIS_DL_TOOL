# 用户手册

## 典型分割流程

1. 构建项目，生成动态库或可执行工具。
2. 调用 `GisAi_Init` 初始化库。
3. 使用 `GisAi_RasterSeg_Create` 创建分割对象。
4. 调用 `GisAi_RasterSeg_Run` 执行分割。
5. 释放返回对象，并在结束时调用 `GisAi_Shutdown`。

## 输入与输出

- 栅格输入：GeoTIFF
- 矢量输出：Shapefile 或 GeoJSON
- 点云保存输出：Shapefile 或 GeoJSON 点要素

## 常见说明

- 坐标转换依赖 PROJ 数据文件可用。
- Windows 下的 AI 集成测试当前建议在 `Release` 模式执行。
- 通过 `Load`、`Create` 和算法接口返回的对象，都必须使用匹配的销毁函数释放。
- 若运行集成测试，请先按 [测试指南](testing.md) 准备测试夹具目录。
- 若本地已有测试模型，可通过 `scripts/prepare_test_model.ps1` 或 `scripts/prepare_test_model.sh` 放入标准测试位置。

## 安装后使用

如果你希望在外部 CMake 工程中使用安装后的库，请参考：

- [../examples/installed_package/README.md](../examples/installed_package/README.md)

外部工程的基本调用方式如下：

```cmake
find_package(gis_ai CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE gis_ai::gis_ai)
```
