# GIS AI 算法库

[![Build Status](https://github.com/your-org/gis_ai_lib/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/gis_ai_lib/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

这是一个基于 C++17 的 GIS 算法库，聚焦栅格、矢量、点云处理，以及基于 ONNX Runtime 的遥感栅格分割能力。

## 功能特性

- 基于 ONNX Runtime 的遥感影像语义分割
- 面向矢量、栅格、点云数据的传统 GIS 算法
- 提供纯 C API，便于 C、C#、Python、Java 等语言绑定
- 支持 Windows、Linux、macOS 的跨平台构建预设

## 技术栈

| 组件 | 技术 | 版本 |
|------|------|------|
| 编程语言 | C++ | C++17 |
| 构建系统 | CMake | 3.16+ |
| 包管理 | vcpkg | 最新版 |
| GIS 依赖 | GDAL/GEOS/PROJ | 3.6+/3.11+/9.2+ |
| AI 推理 | ONNX Runtime | 1.16+ |
| 日志 | spdlog | 1.12+ |
| 测试 | GoogleTest | 1.14+ |

## 快速开始

### 前置条件

- CMake 3.16 或更高版本
- 支持 C++17 的编译器
- vcpkg

### 构建

```bash
cmake --preset=dev-windows
cmake --build --preset=dev
```

### 测试

在运行集成测试前，请先准备测试夹具目录结构：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/generate_test_data.ps1
```

Linux/macOS：

```bash
bash scripts/generate_test_data.sh
```

Windows 下如果测试使用了 PROJ，需将 `PROJ_LIB` 指向 vcpkg 中的 PROJ 数据目录：

```powershell
$env:PROJ_LIB = (Resolve-Path "build/dev-windows/vcpkg_installed/x64-windows/share/proj").Path
ctest --test-dir build/dev-windows -C Debug --output-on-failure -E test_ai_integration
ctest --test-dir build/dev-windows -C Release --output-on-failure -R test_ai_integration
```

由于 ONNX 依赖链在 Windows `Debug` 模式下仍存在 schema 注册问题，`test_ai_integration` 当前仅在 `Release` 模式下执行。

当前仓库只跟踪测试夹具目录约定和说明文档，不直接提交生成后的测试资产。

## 项目结构

```text
gis_ai_lib/
|-- CMakeLists.txt
|-- CMakePresets.json
|-- vcpkg.json
|-- include/gis_ai/
|-- src/
|   |-- core/
|   |-- io/
|   |-- gis/
|   |-- ai/
|   `-- fusion/
|-- tests/
|-- examples/
|-- scripts/
|-- docker/
`-- docs/
```

## 模块说明

### Core

- 日志、异常、平台兼容、内存、配置相关基础能力

### IO

- 栅格读写：TIFF
- 矢量读写：Shapefile、GeoJSON
- 点云读写：基于 GDAL/OGR 的点要素方式读写

### GIS

- 矢量：缓冲区、相交、裁剪、简化、拓扑检查
- 栅格：重采样、归一化、裁剪、阈值分割、镶嵌
- 点云：过滤、降采样
- 坐标转换：基于 PROJ 的 CRS 转换

### AI

- ONNX Runtime 集成
- 模型管理
- 栅格预处理与后处理
- 掩膜转多边形

### Fusion

- 端到端栅格分割流程
- 批量文件处理

## C API 示例

```cpp
#include <gis_ai/gis_ai.h>

int main() {
    if (GisAi_Init(nullptr) != 0) {
        return 1;
    }

    GisAiRasterSeg* seg = GisAi_RasterSeg_Create("test_data/models/test_seg_model.onnx");
    if (!seg) {
        GisAi_Shutdown();
        return 1;
    }

    int ret = GisAi_RasterSeg_Run(
        seg,
        "test_data/raster/test_100x100.tif",
        "test_data/raster/test_seg_output.tif",
        "test_data/vector/test_seg_output.shp");

    GisAi_RasterSeg_Destroy(seg);
    GisAi_Shutdown();
    return ret;
}
```

## 文档索引

- [项目进度](docs/PROJECT_PROGRESS.md)
- [架构设计](docs/architecture.md)
- [API 参考](docs/api_reference.md)
- [用户手册](docs/user_manual.md)
- [测试指南](docs/testing.md)
- [贡献说明](CONTRIBUTING.md)

## 安装后消费示例

仓库提供了安装包消费示例工程：

- [examples/installed_package/README.md](examples/installed_package/README.md)

该示例用于验证安装后的包是否能够通过 `find_package(gis_ai CONFIG REQUIRED)` 和 `gis_ai::gis_ai` 正常被外部工程引用。

## 开发环境

```bash
docker build -t gis_ai_dev -f docker/Dockerfile.dev .
docker run -it gis_ai_dev bash
```

## 许可证

本项目采用 MIT License。详见 [LICENSE](LICENSE)。
