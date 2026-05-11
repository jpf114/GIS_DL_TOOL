# GIS AI TOOL

[![Build Status](https://github.com/your-org/gis_ai_lib/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/gis_ai_lib/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

基于深度学习的遥感智能解译工具，集成传统 GIS 算法与 AI 推理能力，提供 CLI 和 GUI 双入口。

## 功能特性

- **大图分割**：滑窗推理 + 重叠区混合（None/Linear/Gaussian）+ 矢量化
- **模型推理**：基于 ONNX Runtime 的遥感影像语义分割
- **数据预处理**：栅格重采样、归一化、裁剪
- **矢量处理**：简化、缓冲区、裁剪
- **栅格处理**：镶嵌、阈值分割、掩膜转矢量
- **批量处理**：多线程批量推理

## 技术栈

| 组件 | 技术 | 版本 |
|------|------|------|
| 编程语言 | C++ | C++17 |
| 构建系统 | CMake | 3.21+ |
| 包管理 | vcpkg | 最新版 |
| GIS 依赖 | GDAL / GEOS / PROJ | 3.6+ / 3.11+ / 9.2+ |
| AI 推理 | ONNX Runtime | 1.16+ |
| GUI 框架 | Qt6 | 6.x（Widgets + Svg + Sql） |
| 日志 | spdlog | 1.12+ |
| 测试 | GoogleTest | 1.14+ |

## 快速开始

### 前置条件

- CMake 3.21 或更高版本
- 支持 C++17 的编译器（MSVC 2019+ / GCC 9+ / Clang 10+）
- vcpkg（设置 `VCPKG_ROOT` 环境变量）

### 构建

```bash
# Windows
cmake --preset=dev-windows
cmake --build --preset=dev

# Linux
cmake --preset=dev-linux
cmake --build --preset=dev
```

### 构建 GUI

```bash
cmake -B build -S . -DGIS_AI_BUILD_GUI=ON ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### 测试

```powershell
# Windows
$env:PROJ_LIB = (Resolve-Path "build/dev-windows/vcpkg_installed/x64-windows/share/proj").Path
ctest --test-dir build/dev-windows -C Debug --output-on-failure -E test_ai_integration
ctest --test-dir build/dev-windows -C Release --output-on-failure -R test_ai_integration
```

## 使用方法

### CLI

```bash
# 单图分割
gis_ai_cli segment model.onnx input.tif output_seg.tif output_seg.shp

# 批量分割
gis_ai_cli batch model.onnx input_dir/ output_dir/ --threads 4

# 使用配置文件
gis_ai_cli run task_config.json

# 生成默认配置模板
gis_ai_cli generate-config config.json
```

CLI 选项：

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `--tile-size <int>` | 切片大小 | 512 |
| `--stride <int>` | 滑窗步长 | 384 |
| `--blend <mode>` | 融合模式（none/linear/gaussian） | gaussian |
| `--threads <int>` | 批量处理线程数 | 1 |
| `--no-shp` | 不输出矢量文件 | — |
| `--verbose` | 详细日志 | — |

### GUI

```bash
gis-ai-gui
```

GUI 提供图形化的任务配置、执行与进度监控界面，支持任务队列、进度回调和结果查看。

### C API

```cpp
#include <gis_ai/gis_ai.h>

int main() {
    if (GisAi_Init(nullptr) != 0) return 1;

    GisAiRasterSeg* seg = GisAi_RasterSeg_Create("model.onnx");
    int ret = GisAi_RasterSeg_Run(seg, "input.tif", "output.tif", "output.shp");

    GisAi_RasterSeg_Destroy(seg);
    GisAi_Shutdown();
    return ret;
}
```

## 项目结构

```text
GIS_DL_TOOL/
├── CMakeLists.txt
├── CMakePresets.json
├── include/gis_ai/          # 公共头文件
├── src/
│   ├── core/                # 基础能力（日志、异常、配置）
│   ├── io/                  # 数据读写（栅格、矢量、点云）
│   ├── gis/                 # GIS 算法（栅格、矢量、坐标转换）
│   ├── ai/                  # AI 推理（ONNX、预处理、后处理）
│   ├── fusion/              # 融合层（大图分割、批量处理）
│   ├── gui/                 # Qt6 GUI
│   └── cli/                 # 命令行入口
├── tests/
├── examples/
├── tools/
└── docs/
    ├── 架构设计文档.md
    ├── 用户手册.md
    └── 算法说明/            # 算法文档目录
```

## 文档索引

- [架构设计文档](docs/架构设计文档.md)
- [用户手册](docs/用户手册.md)
- [算法说明总览](docs/算法说明/总览.md)
- [API 参考](docs/api_reference.md)
- [测试指南](docs/testing.md)
- [贡献说明](CONTRIBUTING.md)

## 许可证

本项目采用 MIT License。详见 [LICENSE](LICENSE)。
