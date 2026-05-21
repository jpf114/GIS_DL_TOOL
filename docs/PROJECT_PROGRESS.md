# Project Progress

Last updated: 2026-05-21

## Summary

`GIS_DL_TOOL` 已完成产品化核心阶段，从"算法库雏形"升级为**可工作的遥感智能解译引擎**。遥感大图语义分割全流程已打透，GUI/CLI 双入口可用，Windows 安装包可正常构建、安装和运行。

## Build & Test Status

| 项目 | 状态 |
|------|------|
| Debug 构建 + 测试 | ✅ 15/15 通过 |
| Release 构建 + 测试 | ✅ 17/17 通过 |
| Release 安装 | ✅ 通过 |
| CLI 启动 | ✅ `gis_ai_cli.exe help` 正常 |
| GUI self-test | ✅ `gis-ai-gui.exe --self-test` 正常 |
| GUI 启动 | ✅ 进程存活验证通过 |
| 下游 find_package 消费 | ✅ 通过 |
| CPACK NSIS 打包 | ✅ 配置就绪（需安装 NSIS 生成安装包） |
| vcpkg manifest 模式 | ✅ 项目内嵌 vcpkg 优先，同事可一键初始化 |

## Core Capabilities

### 遥感大图语义分割全流程
- 滑窗切片推理（可配置 tile_size / stride）
- Overlap 融合（None / Linear / Gaussian 三种模式）
- mask_threshold 阈值过滤生效
- NoData 像素检测与标记保留
- 小斑块过滤 → 多边形简化 → 拓扑修复 → 属性回写
- class_id 从 config.target_class 正确传递
- 批量处理（BatchProcessor 使用 LargeImageSeg）

### GUI
- Qt GUI 与 GIS_TOOL 对齐：任务队列、SQLite 持久化、任务中心
- gui_data_support 全平台级能力：数据检测、自动填充、输出路径推导、文件对话框、参数文本、结果本地化、参数校验、无效参数高亮
- gis_gui_common 共享 UI 组件库
- GeoPackage 输出支持
- i18n 国际化基础设施（tr()/translate() 全量覆盖）
- Config 与 SettingsManager 双轨配置统一（syncFromCoreConfig 桥接）
- 关于对话框显示正确版本号

### CLI
- segment / inference / batch / generate-config / version / help 子命令
- 支持 `--version` / `-V` 标志
- GDAL/PROJ 运行时数据路径自动初始化
- JSON 运行报告（--report）
- 结构化错误码体系（1xxx=IO, 2xxx=模型, 3xxx=算法, 4xxx=配置, 5xxx=内存, 6xxx=参数, 7xxx=任务）

### SDK
- 共享库 + 静态库输出
- C API 公共接口
- 下游 find_package 消费验证通过

## Alignment with GIS_TOOL

- P0: 全部完成
- P1: 全部完成
- P2: 2/3 完成（剩余：共享 shell 片段减少独立漂移实现）

## Packaging

Windows release 安装布局已验证：
- `install/bin` — 可执行文件和运行时 DLL
- `install/bin/platforms` — Qt 平台插件
- `install/bin/sqldrivers` — SQLite 驱动
- `install/share/proj` — PROJ 数据
- `install/share/gdal` — GDAL 数据
- `install/share/icons` — 应用图标

打包方式：
- **CPACK NSIS**：`cpack -G NSIS -C Release`（需安装 NSIS）
- **CPACK ZIP**：`cpack -G ZIP -C Release`
- **独立 NSIS 脚本**：`packaging/nsis/installer.nsi`
- **一键打包脚本**：`packaging/build_installer.ps1`

Windows 版本信息：
- exe 属性页显示产品名、版本号、公司名、版权声明（通过 app.rc.in 自动注入）

## Development Setup

vcpkg manifest 模式确保依赖一致性：
- `vcpkg.json` — 依赖声明（含 baseline 锁定）
- `vcpkg-configuration.json` — overlay 端口配置
- `vcpkg-overlay/` — 自定义端口补丁

新同事开发流程：
```powershell
git clone <repo>
cd GIS_DL_TOOL
.\setup_vcpkg.ps1          # 一键初始化 vcpkg（克隆+bootstrap）
cmake --preset=release     # 配置
cmake --build --preset=release  # 构建
```

toolchain 发现优先级：
1. 项目内嵌 `vcpkg/` 目录（推荐，确保一致性）
2. `VCPKG_ROOT` 环境变量（兼容全局安装）
3. 手动指定 `-DCMAKE_TOOLCHAIN_FILE`

## Open Areas

- P2: 共享 shell 片段进一步减少独立漂移实现
