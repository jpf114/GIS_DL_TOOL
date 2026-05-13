# GIS_DL_TOOL 产品化实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 参考 GIS_TOOL 项目，为 GIS_DL_TOOL 实现编译安装规范化、VSCode 配置和 Qt6 GUI 界面

**架构：** 参考 GIS_TOOL 的插件化架构，GIS_DL_TOOL 的 GUI 采用相同的左导航+右内容区布局，将 AI 推理功能封装为 GUI 插件。编译安装采用 CMakePresets 的 debug/release 分离模式，安装到项目根目录 install 文件夹。

**技术栈：** C++17, CMake 3.21+, vcpkg, Qt6 (Widgets/Svg/Sql), SQLite3, GDAL, ONNX Runtime, GEOS, PROJ

---

## 前置条件：Git 分支整理

### 任务 0：Git 分支整理

**文件：**
- 无代码变更

- [ ] **步骤 1：确认 main 分支领先 origin/main 9 个提交，codex/binary-seg-pipeline 分支有 20+ 独有提交但内容已被 main 覆盖**

运行：`git log --oneline -10`
预期：main 分支包含所有我们的改进提交

- [ ] **步骤 2：推送 main 到远程**

运行：`git push origin main`
预期：成功推送

- [ ] **步骤 3：删除 codex/binary-seg-pipeline 本地分支**

运行：`git branch -D codex/binary-seg-pipeline`
预期：分支已删除

- [ ] **步骤 4：确认只剩 main 分支**

运行：`git branch -a`
预期：只有 `* main` 和 `remotes/origin/main`

---

## 子计划 A：编译安装规范化

### 任务 A1：重构 CMakePresets.json — debug/release 分离

**文件：**
- 修改：`CMakePresets.json`

参考 GIS_TOOL 的 CMakePresets.json，将当前的 `dev-windows` 预设替换为 `debug` 和 `release` 两个预设。

- [ ] **步骤 1：重写 CMakePresets.json**

```json
{
    "version": 3,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 21,
        "patch": 0
    },
    "configurePresets": [
        {
            "name": "base",
            "hidden": true,
            "generator": "Visual Studio 17 2022",
            "architecture": {
                "value": "x64",
                "strategy": "set"
            },
            "toolset": {
                "value": "host=x64",
                "strategy": "set"
            },
            "toolchainFile": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
            "cacheVariables": {
                "CMAKE_INSTALL_PREFIX": "${sourceDir}/install",
                "GIS_AI_BUILD_GUI": "OFF",
                "GIS_AI_BUILD_TESTS": "ON"
            }
        },
        {
            "name": "debug",
            "displayName": "Debug",
            "description": "Debug 构建配置",
            "inherits": "base",
            "binaryDir": "${sourceDir}/build/debug"
        },
        {
            "name": "release",
            "displayName": "Release",
            "description": "Release 构建配置",
            "inherits": "base",
            "binaryDir": "${sourceDir}/build/release"
        }
    ],
    "buildPresets": [
        {
            "name": "debug",
            "configurePreset": "debug",
            "configuration": "Debug"
        },
        {
            "name": "release",
            "configurePreset": "release",
            "configuration": "Release"
        }
    ],
    "testPresets": [
        {
            "name": "debug",
            "configurePreset": "debug",
            "configuration": "Debug"
        },
        {
            "name": "release",
            "configurePreset": "release",
            "configuration": "Release"
        }
    ]
}
```

- [ ] **步骤 2：验证 CMake 配置**

运行：`cmake --preset debug`
预期：配置成功，binaryDir 为 `build/debug`

- [ ] **步骤 3：验证 Release 配置**

运行：`cmake --preset release`
预期：配置成功，binaryDir 为 `build/release`

### 任务 A2：重构 CMakeLists.txt — 安装支持 + GUI 选项

**文件：**
- 修改：`CMakeLists.txt`
- 创建：`cmake/install_runtime_deps.cmake.in`
- 创建：`cmake/gis_ai_runtime_dlls.cmake`

参考 GIS_TOOL 的 CMakeLists.txt，添加：
1. `CMAKE_INSTALL_PREFIX` 默认为 `${CMAKE_SOURCE_DIR}/install`
2. `GIS_AI_BUILD_GUI` 选项
3. vcpkg 全局安装目录变量推导
4. 运行时 DLL 复制逻辑
5. PROJ/GDAL 数据目录复制
6. install() 规则

- [ ] **步骤 1：修改 CMakeLists.txt 顶部，添加安装前缀和选项**

在 `project()` 之后添加：
```cmake
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Default install path" FORCE)
endif()

include(GNUInstallDirs)

option(GIS_AI_BUILD_GUI "Build Qt GUI" OFF)
option(GIS_AI_BUILD_TESTS "Build tests" ON)
```

- [ ] **步骤 2：添加 vcpkg 全局安装目录推导**

参考 GIS_TOOL 的 `GIS_VCPKG_ROOT` 逻辑，添加 `GIS_AI_VCPKG_ROOT`、`GIS_AI_VCPKG_INSTALLED_DIR`、`GIS_AI_VCPKG_PROJ_DATA_DIR`、`GIS_AI_VCPKG_GDAL_DATA_DIR` 等变量。

- [ ] **步骤 3：添加运行时 DLL 复制和 install 规则**

参考 GIS_TOOL 的 `copy_runtime_dlls` target 和 `install_runtime_deps.cmake.in`。

- [ ] **步骤 4：添加 GUI 子目录条件**

```cmake
if(GIS_AI_BUILD_GUI)
    find_package(Qt6 COMPONENTS Core Gui Widgets Svg Sql REQUIRED)
    find_package(SQLite3 REQUIRED)
    add_subdirectory(src/gui)
endif()
```

- [ ] **步骤 5：编译验证**

运行：`cmake --preset debug && cmake --build build/debug --config Debug`
预期：编译成功

- [ ] **步骤 6：安装验证**

运行：`cmake --install build/debug --config Debug`
预期：install 目录下有 bin/、lib/、include/、share/ 目录

- [ ] **步骤 7：Commit**

```bash
git add CMakeLists.txt CMakePresets.json cmake/
git commit -m "build: 重构编译安装配置，支持 debug/release 分离和安装前缀"
```

### 任务 A3：创建 VSCode 配置

**文件：**
- 创建：`.vscode/settings.json`
- 创建：`.vscode/launch.json`
- 创建：`.vscode/tasks.json`
- 创建：`.vscode/extensions.json`

- [ ] **步骤 1：创建 .vscode/settings.json**

```json
{
    "cmake.configureOnOpen": false,
    "cmake.useCMakePresets": "always",
    "cmake.defaultTarget": "gis_ai",
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "files.associations": {
        "*.h": "cpp",
        "*.cpp": "cpp"
    },
    "editor.formatOnSave": true,
    "editor.tabSize": 4
}
```

- [ ] **步骤 2：创建 .vscode/launch.json**

配置 Debug 和 Release 两种启动配置，用于调试 gis_ai_cli 和 gis-ai-gui。

- [ ] **步骤 3：创建 .vscode/tasks.json**

配置 CMake configure/build/install/test 任务。

- [ ] **步骤 4：创建 .vscode/extensions.json**

推荐扩展：CMake Tools, C/C++, Qt tools 等。

- [ ] **步骤 5：Commit**

```bash
git add .vscode/
git commit -m "chore: 添加 VSCode 工作区配置"
```

---

## 子计划 B：GUI 实现

### 任务 B1：创建 GUI 基础框架

**文件：**
- 创建：`src/gui/CMakeLists.txt`
- 创建：`src/gui/main.cpp`
- 创建：`src/gui/mainwindow.h`
- 创建：`src/gui/mainwindow.cpp`
- 创建：`src/gui/style_constants.h`

参考 GIS_TOOL 的 GUI 架构，创建基础框架：
1. MainWindow：左导航+右内容区双栏布局
2. style_constants.h：完整设计系统（颜色、尺寸、全局样式表）
3. main.cpp：Qt 应用入口 + 命令行参数支持

- [ ] **步骤 1：创建 style_constants.h**

完整复制 GIS_TOOL 的设计系统常量，修改命名空间为 `gis_ai::gui`，调整品牌名为 "GIS AI TOOLKIT"。

- [ ] **步骤 2：创建 mainwindow.h/cpp 骨架**

MainWindow 包含：
- NavPanel（左侧导航）
- QTabWidget（右侧内容区，含"功能配置"和"任务中心"两个页签）
- Hero 区域（图标+标题+描述）
- ParamWidget（参数面板）
- 执行控制区
- 结果摘要区

- [ ] **步骤 3：创建 main.cpp**

- [ ] **步骤 4：创建 src/gui/CMakeLists.txt**

- [ ] **步骤 5：编译验证**

运行：`cmake --preset debug -DGIS_AI_BUILD_GUI=ON && cmake --build build/debug --config Debug --target gis-ai-gui`
预期：编译成功，生成 gis-ai-gui.exe

- [ ] **步骤 6：Commit**

```bash
git add src/gui/
git commit -m "feat(gui): 创建 GUI 基础框架（MainWindow + 样式系统）"
```

### 任务 B2：实现导航面板 NavPanel

**文件：**
- 创建：`src/gui/nav_panel.h`
- 创建：`src/gui/nav_panel.cpp`
- 创建：`src/gui/icon_manager.h`
- 创建：`src/gui/icon_manager.cpp`

参考 GIS_TOOL 的 NavPanel，实现：
1. 暗色侧边栏（250px 宽）
2. 顶部品牌卡片
3. 功能分类按钮（可展开/折叠）
4. 子功能按钮
5. IconManager 单例（SVG 图标渲染 + 缓存）

- [ ] **步骤 1：创建 icon_manager.h/cpp**

- [ ] **步骤 2：创建 nav_panel.h/cpp**

- [ ] **步骤 3：复制图标资源**

从 GIS_TOOL 的 `resources/icons/` 复制 SVG 图标到 `resources/icons/`

- [ ] **步骤 4：编译验证**

- [ ] **步骤 5：Commit**

```bash
git add src/gui/nav_panel.* src/gui/icon_manager.* resources/
git commit -m "feat(gui): 实现导航面板和图标管理器"
```

### 任务 B3：实现参数面板 ParamWidget + ParamCardWidget

**文件：**
- 创建：`src/gui/param_widget.h`
- 创建：`src/gui/param_widget.cpp`
- 创建：`src/gui/param_card_widget.h`
- 创建：`src/gui/param_card_widget.cpp`
- 创建：`src/gui/crs_dialog.h`
- 创建：`src/gui/crs_dialog.cpp`

参考 GIS_TOOL 的参数面板，实现：
1. ParamWidget：输入/输出/高级三组卡片容器
2. ParamCardWidget：单张参数卡片（标题头+参数行列表）
3. 参数控件工厂：Enum/Bool/Int/Double/FilePath/DirPath/CRS/Extent
4. CRS 对话框

- [ ] **步骤 1：创建 param_card_widget.h/cpp**

- [ ] **步骤 2：创建 param_widget.h/cpp**

- [ ] **步骤 3：创建 crs_dialog.h/cpp**

- [ ] **步骤 4：编译验证**

- [ ] **步骤 5：Commit**

```bash
git add src/gui/param_* src/gui/crs_dialog.*
git commit -m "feat(gui): 实现参数面板（输入/输出/高级分组+控件工厂）"
```

### 任务 B4：实现任务中心 TaskCenterPage

**文件：**
- 创建：`src/gui/task_center_page.h`
- 创建：`src/gui/task_center_page.cpp`
- 创建：`src/gui/task_manager.h`
- 创建：`src/gui/task_manager.cpp`
- 创建：`src/gui/task_database.h`
- 创建：`src/gui/task_database.cpp`
- 创建：`src/gui/task_runner.h`
- 创建：`src/gui/task_runner.cpp`

参考 GIS_TOOL 的任务中心，实现：
1. TaskCenterPage：任务列表+日志终端
2. TaskManager：任务生命周期管理
3. TaskDatabase：SQLite 持久化
4. TaskRunner：异步执行引擎

- [ ] **步骤 1：创建 task_database.h/cpp**

- [ ] **步骤 2：创建 task_manager.h/cpp**

- [ ] **步骤 3：创建 task_runner.h/cpp**

- [ ] **步骤 4：创建 task_center_page.h/cpp**

- [ ] **步骤 5：编译验证**

- [ ] **步骤 6：Commit**

```bash
git add src/gui/task_*
git commit -m "feat(gui): 实现任务中心（SQLite持久化+异步执行+日志终端）"
```

### 任务 B5：实现执行引擎和进度报告

**文件：**
- 创建：`src/gui/execute_worker.h`
- 创建：`src/gui/execute_worker.cpp`
- 创建：`src/gui/qt_progress_reporter.h`
- 创建：`src/gui/qt_progress_reporter.cpp`
- 创建：`src/gui/progress_dialog.h`
- 创建：`src/gui/progress_dialog.cpp`
- 创建：`src/gui/gdal_config.h`
- 创建：`src/gui/gdal_config.cpp`
- 创建：`src/gui/settings_manager.h`
- 创建：`src/gui/settings_manager.cpp`

- [ ] **步骤 1：创建 gdal_config.h/cpp**

- [ ] **步骤 2：创建 qt_progress_reporter.h/cpp**

- [ ] **步骤 3：创建 execute_worker.h/cpp**

- [ ] **步骤 4：创建 progress_dialog.h/cpp**

- [ ] **步骤 5：创建 settings_manager.h/cpp**

- [ ] **步骤 6：编译验证**

- [ ] **步骤 7：Commit**

```bash
git add src/gui/execute_worker.* src/gui/qt_progress_reporter.* src/gui/progress_dialog.* src/gui/gdal_config.* src/gui/settings_manager.*
git commit -m "feat(gui): 实现执行引擎、进度报告和设置管理"
```

### 任务 B6：实现 AI 功能插件适配层

**文件：**
- 创建：`src/gui/gui_data_support.h`
- 创建：`src/gui/gui_data_support.cpp`

将 GIS_DL_TOOL 的 AI 功能（LargeImageSeg、BatchProcessor 等）适配为 GUI 可用的插件接口。定义每个功能的：
1. ActionUiConfig（可见参数、必填参数）
2. 参数显示名/描述覆盖
3. 执行逻辑桥接

- [ ] **步骤 1：创建 gui_data_support.h/cpp**

定义 AI 功能的 ActionUiConfig 映射，包括：
- 大图分割（segment）：模型路径、输入影像、输出路径、tile_size、stride、blend_mode 等
- 批量处理（batch）：输入目录、模型路径、输出目录等

- [ ] **步骤 2：编译验证**

- [ ] **步骤 3：Commit**

```bash
git add src/gui/gui_data_support.*
git commit -m "feat(gui): 实现 AI 功能插件适配层"
```

### 任务 B7：MainWindow 完整集成

**文件：**
- 修改：`src/gui/mainwindow.h`
- 修改：`src/gui/mainwindow.cpp`

将所有组件集成到 MainWindow：
1. 插件加载和导航填充
2. 参数面板联动
3. 执行按钮和批量处理
4. 派生参数自动推断
5. 任务中心交互
6. 拖放支持

- [ ] **步骤 1：完善 MainWindow 的 onPluginSelected/onSubFunctionSelected**

- [ ] **步骤 2：实现执行逻辑（同步+异步）**

- [ ] **步骤 3：实现派生参数同步**

- [ ] **步骤 4：实现拖放支持**

- [ ] **步骤 5：编译验证**

- [ ] **步骤 6：运行 GUI 验证**

运行：`build/debug/bin/Debug/gis-ai-gui.exe`
预期：窗口正常显示，导航可点击，参数面板可交互

- [ ] **步骤 7：Commit**

```bash
git add src/gui/mainwindow.*
git commit -m "feat(gui): MainWindow 完整集成（插件加载+执行+任务中心）"
```

### 任务 B8：GUI 安装和打包

**文件：**
- 修改：`src/gui/CMakeLists.txt`
- 修改：`CMakeLists.txt`

添加 GUI 的安装规则：
1. 复制 Qt 运行时依赖
2. 复制图标资源
3. 复制 PROJ/GDAL 数据
4. 创建可分发包

- [ ] **步骤 1：添加 GUI 安装规则**

- [ ] **步骤 2：验证完整安装流程**

运行：`cmake --install build/release --config Release`
预期：install/bin/ 下有 gis-ai-gui.exe 及所有依赖 DLL

- [ ] **步骤 3：Commit**

```bash
git add src/gui/CMakeLists.txt CMakeLists.txt cmake/
git commit -m "build: 添加 GUI 安装和运行时依赖打包"
```

---

## 执行顺序

1. **任务 0** → Git 分支整理（5 分钟）
2. **任务 A1** → CMakePresets 重构（10 分钟）
3. **任务 A2** → CMakeLists.txt 安装支持（20 分钟）
4. **任务 A3** → VSCode 配置（10 分钟）
5. **任务 B1** → GUI 基础框架（30 分钟）
6. **任务 B2** → NavPanel + IconManager（30 分钟）
7. **任务 B3** → ParamWidget + ParamCardWidget（40 分钟）
8. **任务 B4** → TaskCenterPage（30 分钟）
9. **任务 B5** → 执行引擎和进度报告（20 分钟）
10. **任务 B6** → AI 功能插件适配（20 分钟）
11. **任务 B7** → MainWindow 完整集成（30 分钟）
12. **任务 B8** → GUI 安装和打包（15 分钟）

**总计：约 12 个任务，预计 4-5 小时工作量**
