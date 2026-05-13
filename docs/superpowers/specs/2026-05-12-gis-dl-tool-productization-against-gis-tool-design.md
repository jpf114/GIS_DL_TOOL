# GIS_DL_TOOL 对齐 GIS_TOOL 的产品化设计

**日期：** 2026-05-12

**目标：** 以 `D:\Code\MyProject\GIS_TOOL` 为母版，对 `GIS_DL_TOOL` 的 Windows 构建安装链路、GUI 外壳和工程闭环进行系统收敛，使其成为“同平台、不同业务”的产品变体，而不是独立演化的另一套工具。

**适用范围：**
- 当前阶段仅面向 Windows
- 以 Visual Studio 2022 + vcpkg + Qt6 + GDAL/PROJ/GEOS/ONNX Runtime 为标准环境
- 本文档同时承担“现状诊断 + 对齐设计 + 分阶段整改总纲”三种职责

---

## 1. 背景与约束

用户已明确补充以下前提：

1. 当前项目只在 Windows 下使用，不以跨平台为近期目标。
2. `GIS_DL_TOOL` 需要参考 `D:\Code\MyProject\GIS_TOOL`：
   - 构建与安装逻辑要对齐：`build/debug`、`build/release` 双目录构建，最小依赖复制，安装到 `install/`
   - GUI 除业务功能外整体都应与 `GIS_TOOL` 一致

因此，本次设计不再把“跨平台不足”视为问题，而把重点放在：

- Windows 产品化链路是否与 `GIS_TOOL` 同构
- GUI 外壳是否真正是同一平台的业务变体
- 当前项目已有能力能否被稳定构建、测试、安装和消费

---

## 2. 当前项目现状总诊断

### 2.1 项目总体状态

`GIS_DL_TOOL` 已经不是概念验证项目，而是一个具有完整模块体系的 GIS + AI C++ 工程。当前仓库具备：

- 公共库与 C API
- CLI 工具
- Qt GUI
- AI 推理链路
- 传统 GIS 算法模块
- 大图分割与批处理
- 测试工程
- 安装导出逻辑
- 中英文混合但逐渐中文化的文档体系

从代码结构看，当前项目已经形成以下模块边界：

- `src/core/`：日志、异常、配置、平台、内存
- `src/io/`：栅格、矢量、点云读写
- `src/gis/`：矢量/栅格/点云/投影算法
- `src/ai/`：ONNX Runtime、预处理、后处理、掩膜转面
- `src/fusion/`：大图分割、批处理、任务配置
- `src/cli/`：命令行入口
- `src/gui/`：图形界面、任务中心、SQLite 持久化、进度反馈
- `tests/`：单元测试、集成测试、GUI smoke test

### 2.2 已完成能力与阶段成果

结合代码、文档和现有构建产物，当前已落地的能力包括：

- 基于 ONNX Runtime 的遥感影像分割推理
- 大图滑窗分割、重叠融合、掩膜转矢量
- 批量分割任务
- 栅格读写、矢量读写、点云读写
- 矢量缓冲、裁剪、简化、拓扑检查等 GIS 算法
- 栅格裁剪、归一化、阈值、镶嵌等 GIS 算法
- C API 暴露基础 IO、GIS、AI、分割和坐标转换接口
- GUI 的主窗口、导航、参数卡片、任务中心、任务数据库、进度对话框
- `build/debug`、`build/release` 和 `install/` 目录框架

阶段性目标上，项目已经达成：

1. 从算法代码升级为可构建的工程化仓库
2. 从单入口扩展到 `C API + CLI + GUI`
3. 从小图推理推进到大图分割与批处理
4. 从纯功能实现进入产品化收口阶段

### 2.3 当前主要挑战

当前最大的挑战不是“缺少功能”，而是“已有能力尚未完成稳定收口”。主要表现为：

1. 工程闭环弱于功能闭环
2. 构建安装体系与 `GIS_TOOL` 已接近但未完全同构
3. GUI 虽已明显参考 `GIS_TOOL`，但还未达到“除业务外整体一致”的严格标准
4. 测试、日志、运行时依赖仍对本地环境和目录行为过于敏感
5. 文档口径与当前仓库实际行为尚未完全对齐

### 2.4 技术债务与工程风险

#### A. 工程化技术债

- 测试依赖日志文件可写，导致运行环境稍有不同就成片失败
- Debug/Release 运行时依赖完整性不一致
- 安装后外部消费链路缺少系统验证闭环
- 文档中的构建/测试命令与当前 preset、路径存在滞后

#### B. 架构与接口技术债

- C API 暴露面较大，但缺少针对 ABI、所有权、错误处理的专项验证
- GUI 与业务执行桥接耦合度偏高，部分逻辑仍散落在窗口层
- 任务执行链在 `GIS_TOOL` 中已形成 `TaskRunner` 队列模型，而 `GIS_DL_TOOL` 目前更偏局部线程驱动

#### C. 交付风险

- 仓库当前是脏工作区，说明仍处于活跃整理中
- 现有“可运行”不等于“可稳定交付”
- 若不尽快完成母版对齐，后续两个项目会形成维护分叉

---

## 3. 问题分级与影响评估

### 3.1 P0：必须优先解决的问题

#### P0-1 测试体系当前不可稳定通过

当前 `Release` 下 `ctest -C Release --output-on-failure` 能找到测试目标，但测试会被日志文件写入失败级联打断；`Debug` 下还存在运行时依赖不完整导致的异常退出。

**影响范围：**
- 阻碍回归验证
- 阻碍构建验收
- 阻碍安装后验证

**结论：** 这是当前最核心的工程阻塞项之一。

#### P0-2 构建安装逻辑虽接近母版，但未完成严格同构

`GIS_DL_TOOL` 顶层 `CMakeLists.txt`、`CMakePresets.json`、`src/gui/CMakeLists.txt` 已明显参考 `GIS_TOOL`，但仍存在以下差距：

- GUI 默认开关策略不同
- 安装后 runtime dependency 扫描策略弱于 `GIS_TOOL`
- Qt `sqldrivers` 复制与安装链不完整
- 可执行产物、插件产物、资源产物的布局规范仍不完全一致

**影响范围：**
- 安装包结构不稳定
- Debug/Release 行为不一致
- 后续维护难以统一

#### P0-3 GUI 外壳尚未实现真正的平台同构

当前 `GIS_DL_TOOL` GUI 已具备与 `GIS_TOOL` 接近的基本结构，但在以下方面仍未完全一致：

- `main.cpp` 启动参数、自测/截图/状态文件支持不足
- 任务执行采用局部线程模式，未完全对齐 `TaskRunner` 队列模型
- 任务数据结构、结果消息、本地化规则、参数自动派生策略不及 `GIS_TOOL` 完整
- `gui_data_support.cpp` 的数据驱动配置能力明显弱于母版

**影响范围：**
- 两个工具在交互层面会逐渐偏离
- 后续复用 `GIS_TOOL` 优化成果的成本提高

### 3.2 P1：高优先但可在 P0 后推进的问题

#### P1-1 文档与当前行为不一致

典型问题包括：

- 测试文档仍引用旧路径/旧 preset 口径
- 用户手册和 API 文档对当前安装、测试和入口能力描述不够完整
- 项目进度文档偏“阶段陈述”，缺少对 `GIS_TOOL` 对齐目标的正式表达

#### P1-2 C API 验证不足

虽然接口面已经比较大，但现有测试体系主要偏向内部 C++ 类，不足以证明对外接口的稳定性与使用体验。

#### P1-3 日志与运行时目录策略过刚

默认日志文件路径与工作目录绑定过紧，导致测试和受限环境脆弱。

### 3.3 P2：中长期整理项

#### P2-1 大图分割性能和内存模型仍偏保守

当前实现已满足功能，但距离稳定生产批处理还有优化空间。

#### P2-2 代码组织可继续向母版抽象靠拢

特别是 GUI 与执行桥接部分，需要进一步从“窗口主导”转向“数据驱动 + 队列执行”。

---

## 4. GIS_TOOL 母版规则提炼

本节不只是列差异，而是提炼 `GIS_TOOL` 当前已经成立的平台规则。

### 4.1 构建规则

`GIS_TOOL` 的 Windows 母版构建规则为：

1. 使用 `CMakePresets.json` 定义 `base / debug / release`
2. 构建目录固定为：
   - `build/debug`
   - `build/release`
3. 安装前缀固定为：
   - `install/`
4. 所有依赖基于全局 `vcpkg` 安装目录推导
5. 顶层在 `project()` 之前集成 toolchain

### 4.2 运行时依赖规则

`GIS_TOOL` 使用两层策略管理 runtime：

1. `cmake/gis_runtime_dlls.cmake`
   - 定义最小运行时 DLL 名单
   - 对 Debug/Release 分别复制
2. `cmake/install_runtime_deps.cmake.in`
   - 安装时优先使用 `GET_RUNTIME_DEPENDENCIES`
   - 失败时回退到最小 DLL 列表

这意味着母版已经形成：

- 构建目录可运行
- 安装目录可运行
- 最小依赖复制有明确标准
- runtime 扫描与兜底并存

### 4.3 安装布局规则

`GIS_TOOL` 的安装布局强调“最小可运行”：

- `install/bin/`：CLI、GUI、主运行时 DLL
- `install/bin/plugins/`：插件 DLL
- `install/bin/platforms/`：Qt platform plugins
- `install/bin/sqldrivers/`：Qt SQLite driver
- `install/share/proj/`
- `install/share/gdal/`
- `install/share/icons/`

### 4.4 GUI 平台规则

`GIS_TOOL` 的 GUI 已经形成清晰平台骨架：

- 左导航 + 右主面板 + Tab 任务中心
- Hero 区 + 参数卡片 + 执行区 + 状态栏
- SQLite 任务数据库
- 任务队列执行器 `TaskRunner`
- `gui_data_support.cpp` 驱动参数文本、可见性、必填规则、自动输出推导、业务校验
- `main.cpp` 支持 GUI 自动化与截图/状态文件输出

### 4.5 GUI 与业务的分层规则

母版的核心不是“界面长得像”，而是“外壳稳定、业务可插拔”：

- 主窗口与导航负责平台体验
- 参数模型和动作描述由数据驱动
- 执行链路由统一任务运行器承载
- 插件/业务功能只负责动作实现与参数规范

这是 `GIS_DL_TOOL` 对齐时必须保留的原则。

---

## 5. GIS_DL_TOOL 当前对齐情况判断

### 5.1 已对齐项

以下部分已经明显沿用了 `GIS_TOOL` 的方向：

- `CMakePresets.json` 已采用 `base / debug / release`
- 顶层 `CMakeLists.txt` 已有 `install/`、vcpkg 目录推导、runtime 复制逻辑
- `src/gui/` 的文件组织已经与母版高度相似：
  - `mainwindow`
  - `nav_panel`
  - `param_widget`
  - `task_center_page`
  - `task_database`
  - `execute_worker`
  - `qt_progress_reporter`
  - `settings_manager`
  - `gdal_config`
- `style_constants.h` 已高度接近母版样式体系
- 任务中心与 SQLite 持久化的基础形态已建立

### 5.2 部分对齐项

以下部分方向一致，但成熟度仍不足：

- Qt platform DLL 安装：已有，但不如 `GIS_TOOL` 完整
- 资源目录复制：已有，但与母版布局仍需核对
- GUI 主窗口布局：接近，但仍缺少母版自动化入口和部分交互细节
- 任务持久化：结构相似，但字段与结果表达能力弱于母版
- 参数配置数据驱动：存在，但覆盖度远低于母版

### 5.3 未对齐项

以下是目前仍存在明显偏差的部分：

- `main.cpp` 缺少母版的截图、自测、参数注入、执行状态输出能力
- 尚未引入母版级 `TaskRunner` 队列执行模型
- `gui_data_support.cpp` 的参数文本、派生逻辑、动作校验能力不足
- 安装时 runtime dependency 自动扫描和 fallback 机制弱于母版
- Qt `sqldrivers` 安装链未严格同构
- 文档尚未正式纳入“以 GIS_TOOL 为母版”的产品化叙事

---

## 6. 对齐设计

## 6.1 构建安装对齐设计

### 设计目标

使 `GIS_DL_TOOL` 的 Windows 构建和安装行为与 `GIS_TOOL` 同构，并确保：

- `build/debug` 可用于调试和本地运行
- `build/release` 可用于验收和打包
- `install/` 为最小可运行安装目录

### 设计原则

1. 不再自行发明另一套安装逻辑，优先复用母版模式
2. `GIS_DL_TOOL` 与 `GIS_TOOL` 的差异仅保留在业务依赖层
3. 对运行时依赖采用“扫描优先，最小清单兜底”

### 设计内容

#### A. 顶层 CMake 同构化

需要让 `GIS_DL_TOOL` 顶层构建在结构上尽量对齐 `GIS_TOOL`：

- 保持 `base/debug/release` preset 结构
- 保持 `install/` 作为默认安装前缀
- 统一全局 `vcpkg` 目录推导方式
- 补齐 Qt `platforms`、`sqldrivers`、数据目录、icons 的安装路径定义

#### B. runtime DLL 管理升级

当前 `GIS_DL_TOOL` 虽有 `cmake/gis_ai_runtime_dlls.cmake`，但建议对齐为：

- 构建时复制最小 DLL 集
- 安装时优先扫描真实依赖
- 安装时保留最小清单兜底

这将直接提升：

- Debug/Release 一致性
- 安装目录可运行性
- 后续依赖升级可维护性

#### C. GUI 安装链路同构

`src/gui/CMakeLists.txt` 应对齐母版：

- 复制 Qt platforms
- 复制 Qt SQLite driver
- 复制 icons 到可执行旁
- 安装时将 GUI 运行所需目录布局固定化

### 验收标准

- `cmake --preset debug` 成功
- `cmake --preset release` 成功
- `cmake --build --preset debug` 成功
- `cmake --build --preset release` 成功
- `cmake --install build/release --config Release` 后，`install/` 中 GUI 和 CLI 可直接运行

## 6.2 GUI 外壳对齐设计

### 设计目标

将 `GIS_DL_TOOL` GUI 明确定义为 `GIS_TOOL` 的业务变体：

- 平台外壳相同
- 交互习惯相同
- 任务中心相同
- 参数面板组织方式相同
- 仅业务动作、参数文本和执行桥接不同

### 设计原则

1. 除业务功能外，优先完全复用母版的 GUI 行为
2. 尽量减少“看起来相似但内部不同”的分叉
3. 把业务差异集中到 `gui_data_support` 和执行桥接

### 设计内容

#### A. 主窗口与启动入口对齐

`src/gui/main.cpp` 与 `mainwindow.cpp` 需要向 `GIS_TOOL` 收敛：

- 支持自测模式
- 支持截图/状态文件/参数注入等自动化入口
- 统一字体、GDAL/PROJ runtime 初始化逻辑
- 统一窗口标题、应用名、启动环境处理

#### B. 任务执行模型对齐

建议将 `GIS_DL_TOOL` 当前较局部的线程执行模型升级为类似母版的：

- `TaskRunner` 统一队列
- `ExecuteWorker` 仅负责单任务执行
- `TaskManager` 负责记录生命周期
- `TaskDatabase` 负责持久化和日志

这样可以解决：

- 同时执行多个任务时的一致性
- 重跑/编辑/队列提交的行为统一
- GUI 自动化和未来回归测试的基础能力

#### C. 参数与动作描述数据驱动对齐

`GIS_TOOL` 的 `gui_data_support.cpp` 非常关键，它不是简单的帮助文件，而是 GUI 平台行为中心。  
`GIS_DL_TOOL` 需要把以下能力向它看齐：

- 参数显示名与描述管理
- 子功能可见参数和必填参数管理
- 自动输出路径推导
- 数据探测与自动填充
- 动作级参数校验
- 结果消息本地化

对 `GIS_DL_TOOL` 来说，业务差异应主要保留在：

- 分割/批处理相关动作定义
- 模型路径、输入栅格、输出栅格/矢量、tile/stride/blend 等 AI 参数规则

#### D. 样式与交互细节对齐

尽管样式文件已较接近，仍应明确要求以下内容与母版保持同步：

- `style_constants.h`
- `nav_panel`
- `task_center_page`
- `progress_dialog`
- Hero 区、执行区、状态栏、日志终端的视觉和交互行为

### 验收标准

- 主界面布局、导航、任务中心、参数卡片与母版视觉和行为一致
- 自测和自动化启动入口可用
- 任务重跑、编辑、清日志、清历史等操作行为稳定
- 仅业务功能树和参数内容不同

## 6.3 工程稳定性整改设计

### A. 日志策略整改

当前默认日志初始化对工作目录和写权限过敏，需要改为：

- 允许控制台-only 模式
- 文件日志不可写时降级而非抛异常中断核心流程
- 测试默认使用可控临时目录或显式禁用文件日志

### B. 测试执行环境整改

需要系统梳理：

- 测试工作目录
- 运行时 DLL 完整性
- `GDAL_DATA` / `PROJ_DATA`
- `test_data` 和 ONNX 模型准备方式

### C. 安装后消费验证整改

需要把下列链路正式纳入验收：

- `cmake --install`
- 安装目录 CLI 启动
- 安装目录 GUI 启动
- 安装后 C API / 外部 `find_package` 冒烟验证

## 6.4 文档收口设计

文档需要统一到新的产品化叙事：

- 当前仅面向 Windows
- `GIS_TOOL` 是产品母版
- `GIS_DL_TOOL` 是 AI/GIS 业务变体
- 构建、测试、安装说明必须与真实路径一致

需优先收口的文档包括：

- `README.md`
- `docs/testing.md`
- `docs/PROJECT_PROGRESS.md`
- `docs/user_manual.md`
- `docs/api_reference.md`

---

## 7. 分阶段实施计划

## 阶段一：构建安装对齐

**目标：** 完成 `GIS_TOOL` 式 Windows 构建安装链路

**具体任务：**
- 对齐顶层 `CMakeLists.txt`
- 对齐 `CMakePresets.json`
- 升级 `cmake/gis_ai_runtime_dlls.cmake`
- 升级 `cmake/install_runtime_deps.cmake.in`
- 对齐 `src/gui/CMakeLists.txt` 中 Qt runtime 安装逻辑

**预期成果：**
- `build/debug`
- `build/release`
- `install/` 最小可运行目录

**验收标准：**
- Release 安装后 GUI/CLI 能直接运行

## 阶段二：GUI 外壳同构化

**目标：** 让 `GIS_DL_TOOL` 成为 `GIS_TOOL` 的 GUI 平台变体

**具体任务：**
- 对齐 `main.cpp`
- 对齐 `mainwindow.cpp/h`
- 引入或对齐 `TaskRunner`
- 强化 `gui_data_support.cpp`
- 对齐任务中心、任务数据库、进度和结果流转

**预期成果：**
- 同平台体验
- 同平台任务机制
- 差异只集中在业务动作层

**验收标准：**
- 自动化启动、自测、截图、状态输出可用
- 任务流程行为与母版一致

## 阶段三：测试与工程闭环收口

**目标：** 让项目具备稳定验证能力

**具体任务：**
- 调整日志策略
- 修复 Debug/Release 运行时问题
- 明确测试数据准备入口
- 重跑核心测试
- 增补安装后回归验证

**预期成果：**
- 测试结果具有可复现意义
- 安装结果具备可验收意义

**验收标准：**
- 核心测试能稳定执行
- 安装后冒烟验证通过

## 阶段四：文档收口与发布基线

**目标：** 文档与代码现实一致

**具体任务：**
- 更新 README
- 更新测试文档
- 更新项目进度文档
- 明确已知限制和支持范围

**预期成果：**
- 仓库对外叙事统一
- 后续实施有清晰基线

**验收标准：**
- 新开发者按文档可完成构建、运行和验证

---

## 8. 最终建议与决策

本项目不建议继续以“在现有基础上零散修补”的方式推进，而应采用：

**严格母版对齐，业务层最小差异化。**

原因如下：

1. 用户目标已经明确要求与 `GIS_TOOL` 保持平台一致
2. 当前 `GIS_DL_TOOL` 已经在很多地方向母版靠近，继续分叉只会提高维护成本
3. `GIS_TOOL` 已经沉淀出成熟的 Windows 产品化和 GUI 平台规则，复用收益远高于重新设计

因此，后续实施应以如下原则为总纲：

- 不再单独发明 `GIS_DL_TOOL` 的产品外壳
- 把 `GIS_TOOL` 视为统一平台母版
- 把 `GIS_DL_TOOL` 视为 AI/GIS 业务变体
- 先完成构建安装与 GUI 同构，再继续做剩余功能扩展

---

## 9. 结论

`GIS_DL_TOOL` 当前最缺的不是新功能，而是把现有能力真正收敛成：

1. 与 `GIS_TOOL` 同构的 Windows 产品平台
2. 可稳定构建、安装、运行、验证的工程交付物
3. 清晰可维护的“同平台双业务”体系

本设计文档即作为后续实施计划的母文档使用。
