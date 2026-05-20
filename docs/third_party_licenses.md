# GIS AI Tool 第三方依赖许可证汇总

本项目使用以下第三方开源库。分发时须遵守各库的许可证要求。

## 项目许可证

本项目采用 **MIT License**。

## 直接依赖

| 依赖 | 版本 | 许可证 | 用途 | 合规要求 |
|------|------|--------|------|----------|
| GDAL | 3.x | MIT | 栅格/矢量数据 I/O | 保留版权声明 |
| GEOS | 3.x | LGPL-2.1-only | 几何运算引擎 | 动态链接无需开源；静态链接需提供可替换的目标文件 |
| PROJ | 9.x | MIT | 坐标系投影转换 | 保留版权声明 |
| ONNX Runtime | 1.x | MIT | AI 模型推理引擎 | 保留版权声明 |
| nlohmann-json | 3.x | MIT | JSON 解析 | 保留版权声明 |
| spdlog | 1.x | MIT | 高性能日志 | 保留版权声明 |
| Google Test | 1.x | BSD-3-Clause | 单元测试框架 | 保留版权声明和免责条款 |
| Qt6 (qtbase) | 6.x | LGPL-3.0-only / Commercial | GUI 框架（Widgets、SQL） | 动态链接无需开源；静态链接需满足 LGPL 要求 |
| Qt6 (qtsvg) | 6.x | LGPL-3.0-only / Commercial | SVG 支持 | 同上 |
| SQLite3 | 3.x | Public Domain (blessing) | 嵌入式数据库 | 无限制 |

## 间接依赖（通过 vcpkg 引入）

| 依赖 | 许可证 | 引入方式 | 合规要求 |
|------|--------|----------|----------|
| ONNX | Apache-2.0 | vcpkg overlay | 保留版权声明和 NOTICE 文件 |
| Protobuf | BSD-3-Clause | ONNX 的依赖 | 保留版权声明和免责条款 |
| vcpkg-cmake | MIT | vcpkg 构建辅助 | 保留版权声明 |
| vcpkg-cmake-config | MIT | vcpkg 构建辅助 | 保留版权声明 |

## 可选依赖

| 依赖 | 许可证 | 用途 | 说明 |
|------|--------|------|------|
| OpenMP | MIT（随编译器提供） | 并行栅格处理 | GCC/MSVC 运行时自带 |
| Doxygen | GPL-2.0-only | API 文档生成 | 仅构建时使用，不分发 |

## 许可证兼容性分析

### 无风险（宽松许可证）

以下依赖采用 MIT/BSD/Apache/Public Domain 许可证，与项目 MIT 许可证完全兼容，仅需在分发时保留版权声明：

- GDAL、PROJ、ONNX Runtime、nlohmann-json、spdlog、Google Test、SQLite3、ONNX、Protobuf

### 需注意（弱传染性许可证）

以下依赖采用 LGPL 许可证，**动态链接**使用时无需开源项目代码：

- **GEOS** (LGPL-2.1-only) — 当前通过 vcpkg 以动态链接方式引入，合规
- **Qt6** (LGPL-3.0-only) — 当前通过 vcpkg 以动态链接方式引入，合规

> **重要**：如果未来改为静态链接 GEOS 或 Qt6，需提供目标文件（.o/.obj）以允许最终用户重新链接替换这些库。

### 不影响分发

- **Doxygen** (GPL-2.0-only) — 仅用于构建时文档生成，不链接到项目产物中

## 分发清单

发布产品时，须确保以下文件随产品一起分发：

1. **本项目 LICENSE 文件** — MIT 许可证全文
2. **各依赖的版权声明** — 可从 vcpkg 安装目录的 `share/<package>/copyright` 获取
3. **NOTICE 文件**（如有）— Apache-2.0 许可证的依赖（ONNX）要求

建议在发布包中创建 `third-party-licenses/` 目录，存放各依赖的完整许可证文本。
