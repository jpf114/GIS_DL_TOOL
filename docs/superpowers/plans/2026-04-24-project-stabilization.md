# GIS AI Library 稳定化实施计划

> **面向代理执行者：** 实施本计划时，必须使用 `superpowers:subagent-driven-development`（推荐）或 `superpowers:executing-plans` 子技能逐项推进。步骤使用复选框 `- [ ]` 语法跟踪。

**目标：** 通过补齐测试资产入口、统一 CMake 与 CI 行为、验证安装与对外消费链路，使仓库在全新环境中具备可复现的构建与验证能力。

**架构思路：** 当前代码库的模块结构和公共 C API 已经比较完整，因此下一阶段不再以新增功能为重点，而是优先做工程收口。工作顺序从“构建/测试边界”开始，逐步推进到“安装/消费验证”和“跨平台校验”，确保文档中的结论都能从干净仓库中实际复现。

**技术栈：** C++17、CMake、CMake Presets、vcpkg、GoogleTest、GitHub Actions、GDAL、GEOS、PROJ、ONNX Runtime、Python ctypes

---

## 文件范围

- 新增：`docs/superpowers/plans/2026-04-24-project-stabilization.md`
- 新增：`scripts/generate_test_data.ps1`
- 新增：`scripts/generate_test_data.sh`
- 新增：`test_data/README.md`
- 修改：`CMakeLists.txt`
- 修改：`CMakePresets.json`
- 修改：`tests/test_io_integration.cpp`
- 修改：`tests/test_ai_integration.cpp`
- 修改：`scripts/test_ctypes.py`
- 修改：`.github/workflows/ci.yml`
- 修改：`.github/workflows/build.yml`
- 修改：`README.md`
- 修改：`docs/testing.md`
- 修改：`docs/user_manual.md`
- 修改：`docs/PROJECT_PROGRESS.md`

### 任务 1：让测试资产可复现

**涉及文件：**
- 新增：`scripts/generate_test_data.ps1`
- 新增：`scripts/generate_test_data.sh`
- 新增：`test_data/README.md`
- 修改：`tests/test_io_integration.cpp`
- 修改：`tests/test_ai_integration.cpp`
- 修改：`scripts/test_ctypes.py`
- 修改：`README.md`
- 测试：`tests/test_io_integration.cpp`
- 测试：`tests/test_ai_integration.cpp`
- 测试：`scripts/test_ctypes.py`

- [ ] **步骤 1：补充标准测试数据目录说明**

创建 `test_data/README.md`，明确仓库期望的目录结构：

```md
# 测试数据

仓库中的集成测试默认依赖以下目录结构：

- `test_data/raster/test_100x100.tif`
- `test_data/vector/test_polygons.shp`
- `test_data/vector/test_points.geojson`
- `test_data/models/test_seg_model.onnx`

Windows 下使用 `scripts/generate_test_data.ps1`，Linux/macOS 下使用 `scripts/generate_test_data.sh` 准备目录与后续夹具生成入口。
```

- [ ] **步骤 2：增加 Windows 测试夹具生成入口**

创建 `scripts/generate_test_data.ps1`：

```powershell
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$testData = Join-Path $root "test_data"

New-Item -ItemType Directory -Force -Path (Join-Path $testData "raster") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $testData "vector") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $testData "models") | Out-Null

Write-Host "已准备测试夹具目录：$testData"
Write-Host "下一步应扩展此脚本，调用仓库中的 C++ 工具链生成栅格、矢量和 ONNX 测试数据。"
```

- [ ] **步骤 3：增加 Unix 测试夹具生成入口**

创建 `scripts/generate_test_data.sh`：

```bash
#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
mkdir -p "$repo_root/test_data/raster" "$repo_root/test_data/vector" "$repo_root/test_data/models"

echo "已准备测试夹具目录：$repo_root/test_data"
echo "下一步应扩展此脚本，调用仓库中的 C++ 工具链生成栅格、矢量和 ONNX 测试数据。"
```

- [ ] **步骤 4：让集成测试在缺少夹具时给出明确提示**

在 `tests/test_io_integration.cpp` 和 `tests/test_ai_integration.cpp` 中补充统一前置检查：

```cpp
static bool TestDataRootExists() {
    return std::filesystem::exists("test_data");
}

if (!TestDataRootExists()) {
    GTEST_SKIP() << "未找到 test_data 目录，请先运行测试数据准备脚本";
}
```

- [ ] **步骤 5：让 ctypes 验证脚本指向统一的数据准备入口**

调整 `scripts/test_ctypes.py` 的夹具检查逻辑：

```python
if not os.path.exists(TEST_DATA_DIR):
    raise RuntimeError(
        "未找到 test_data 目录。请先运行 scripts/generate_test_data.ps1 或 scripts/generate_test_data.sh。"
    )
```

- [ ] **步骤 6：验证缺少夹具时的失败路径是否清晰**

运行：

```powershell
ctest --test-dir build/dev-windows -C Debug --output-on-failure -R test_io_integration
ctest --test-dir build/dev-windows -C Release --output-on-failure -R test_ai_integration
python scripts/test_ctypes.py
```

预期：
- 缺少夹具时，测试会给出清晰的跳过或初始化错误信息。
- 夹具存在时，测试不会再因路径约定不清导致混乱。

- [ ] **步骤 7：提交**

```bash
git add test_data/README.md scripts/generate_test_data.ps1 scripts/generate_test_data.sh README.md docs/testing.md tests/test_io_integration.cpp tests/test_ai_integration.cpp scripts/test_ctypes.py
git commit -m "构建：补齐测试数据入口与缺失提示"
```

### 任务 2：统一 CMake 行为与构建意图

**涉及文件：**
- 修改：`CMakeLists.txt`
- 修改：`CMakePresets.json`
- 修改：`README.md`
- 修改：`docs/testing.md`
- 测试：`CMakeLists.txt`

- [ ] **步骤 1：明确当前错误行为**

在改动前确认以下问题确实存在：

```text
- 即使 `GIS_AI_BUILD_TESTS=OFF`，`find_package(GTest CONFIG REQUIRED)` 仍然会执行
- `release-*` 预设默认关闭测试，但 CI 仍然继续调用 `ctest`
```

- [ ] **步骤 2：将测试依赖改为条件启用**

将 `CMakeLists.txt` 中的：

```cmake
find_package(GTest CONFIG REQUIRED)
```

改为：

```cmake
if(GIS_AI_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
endif()
```

- [ ] **步骤 3：保持工具与示例不受测试开关影响**

保留现有子目录结构，但确保只有测试路径依赖 GTest：

```cmake
if(GIS_AI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

- [ ] **步骤 4：修正预设，让名称和行为一致**

把 `CMakePresets.json` 中的 release 预设调整为与 CI 预期一致：

```json
"cacheVariables": {
  "CMAKE_BUILD_TYPE": "Release",
  "GIS_AI_BUILD_TESTS": "ON",
  "GIS_AI_BUILD_EXAMPLES": "ON",
  "VCPKG_OVERLAY_PORTS": "${sourceDir}/vcpkg-overlay"
}
```

如果后续需要“只打包不测试”的快速预设，应新增独立的 `package-*` 预设，而不是继续复用 `release-*`。

- [ ] **步骤 5：重新配置，验证开关行为**

运行：

```powershell
cmake --preset=dev-windows
cmake --preset=release-windows
```

预期：
- `dev-windows` 会按开发模式配置测试。
- 如果 CI 要运行 `ctest`，`release-windows` 也必须带测试。
- 后续仍然可以单独增加无测试的打包预设，而不需要强制依赖 GTest。

- [ ] **步骤 6：提交**

```bash
git add CMakeLists.txt CMakePresets.json README.md docs/testing.md
git commit -m "构建：统一预设与测试依赖"
```

### 任务 3：消除 CI 歧义并确保测试真实执行

**涉及文件：**
- 修改：`.github/workflows/ci.yml`
- 修改：`.github/workflows/build.yml`
- 修改：`README.md`
- 修改：`docs/PROJECT_PROGRESS.md`
- 测试：`.github/workflows/ci.yml`
- 测试：`.github/workflows/build.yml`

- [ ] **步骤 1：明确主工作流与补充工作流**

约定：

```text
- `ci.yml` = 核心校验工作流
- `build.yml` = 扩展构建与产物输出工作流
```

将这一约定写进 workflow 名称与注释中，避免后续再次分叉。

- [ ] **步骤 2：在 CI 中显式表达 Windows 下 AI 集成测试的限制**

保留 Windows 下 Debug/Release 分离策略，例如：

```yaml
- name: Test non-AI targets
  run: ctest -C Debug --output-on-failure -E test_ai_integration

- name: Test AI integration in Release
  run: ctest -C Release --output-on-failure -R test_ai_integration
```

- [ ] **步骤 3：避免“没有编测试却仍然执行 ctest”**

如果 workflow 使用了关闭测试的预设，就必须移除 `ctest` 步骤；只要 workflow 要跑 `ctest`，对应预设就必须保证 `GIS_AI_BUILD_TESTS=ON`。

- [ ] **步骤 4：统一依赖基线**

统一 `vcpkgGitCommitId`，保证两套 workflow 使用相同依赖基线：

```yaml
with:
  vcpkgGitCommitId: 42f37e46d37baf7b218966434b91116bddf73461
```

- [ ] **步骤 5：在本地按文本检查 workflow 是否一致**

运行：

```powershell
Get-Content .github/workflows/ci.yml
Get-Content .github/workflows/build.yml
```

预期：
- 必测路径只有一个权威来源。
- 不会再出现未启用测试却继续调用 `ctest` 的情况。
- 两套 workflow 使用相同依赖基线。

- [ ] **步骤 6：提交**

```bash
git add .github/workflows/ci.yml .github/workflows/build.yml README.md docs/PROJECT_PROGRESS.md
git commit -m "持续集成：消除工作流歧义"
```

### 任务 4：验证安装与外部消费链路

**涉及文件：**
- 修改：`README.md`
- 修改：`docs/user_manual.md`
- 修改：`docs/testing.md`
- 修改：`examples/CMakeLists.txt`
- 修改：`cmake/gis_aiConfig.cmake.in`
- 测试：`examples/demo_segmentation.cpp`

- [ ] **步骤 1：明确安装后消费验证目标**

记录以下验收条件：

```text
- `cmake --install` 成功执行
- 下游 CMake 工程可以通过 `find_package(gis_ai CONFIG REQUIRED)` 找到包
- 至少一个示例程序可以链接安装后的包，而不只是源码树内目标
```

- [ ] **步骤 2：增加或修正安装包冒烟示例**

调整 `examples/CMakeLists.txt`，使至少一个示例支持“安装包模式”构建：

```cmake
find_package(gis_ai CONFIG REQUIRED)
add_executable(demo_segmentation demo_segmentation.cpp)
target_link_libraries(demo_segmentation PRIVATE gis_ai::gis_aiTargets)
```

如果当前导出命名空间不一致，就同步修正导出逻辑，让使用者通过稳定的导入目标名（如 `gis_ai::gis_ai`）链接。

- [ ] **步骤 3：必要时修正包导出命名**

调整 `cmake/gis_aiConfig.cmake.in` 和安装导出逻辑，保证公共导入目标可预测：

```cmake
include("${CMAKE_CURRENT_LIST_DIR}/gis_aiTargets.cmake")
check_required_components(gis_ai)
```

- [ ] **步骤 4：运行安装与下游配置验证**

运行：

```powershell
cmake --build --preset=dev
cmake --install build/dev-windows --config Debug --prefix build/install-dev
```

然后创建一个最小下游工程并运行：

```powershell
cmake -S <consumer> -B <consumer-build> -DCMAKE_PREFIX_PATH=D:/Code/MyProject/GIS_DL_TOOL/build/install-dev
cmake --build <consumer-build>
```

预期：
- 安装产物导出了可导入的目标。
- 一个干净的下游工程可以在不依赖源码树内部路径的情况下完成链接。

- [ ] **步骤 5：提交**

```bash
git add README.md docs/user_manual.md docs/testing.md examples/CMakeLists.txt cmake/gis_aiConfig.cmake.in
git commit -m "打包：验证安装后消费链路"
```

### 任务 5：刷新项目状态与发布就绪度

**涉及文件：**
- 修改：`README.md`
- 修改：`docs/PROJECT_PROGRESS.md`
- 修改：`docs/user_manual.md`
- 修改：`docs/testing.md`

- [ ] **步骤 1：把不可复核表述改成可复现表达**

把类似：

```md
128/128 tests passed
```

改成：

```md
最近一次可确认结果：2026-04-23 在维护者环境中验证通过 128/128 项测试。
当前仓库状态下，如需复现该结果，需要先准备测试夹具。
```

- [ ] **步骤 2：区分“已实现”和“当前仓库可复现”**

补充状态表：

```md
| 模块 | 已实现 | 干净仓库可复现 | 说明 |
|------|--------|------------------|------|
| Core/GIS 代码 | 是 | 依赖安装后可验证 | 基本稳定 |
| IO 集成 | 是 | 需先准备测试夹具 | 依赖 `test_data` |
| AI 集成 | 是 | 需先准备夹具，Windows 下需 Release | 受 ONNX Debug 限制 |
| 打包/安装 | 部分完成 | 需补下游消费冒烟验证 | 需要安装链路验证 |
```

- [ ] **步骤 3：增加发布门禁清单**

在 `docs/PROJECT_PROGRESS.md` 末尾追加：

```md
## 发布门禁

- [ ] 新克隆仓库可按文档命令完成配置
- [ ] 可通过仓库脚本准备测试夹具
- [ ] 核心 CI 工作流通过
- [ ] 安装/导出冒烟验证通过
- [ ] README 与测试文档和实际行为一致
```

- [ ] **步骤 4：统一核对所有文档**

运行：

```powershell
Get-Content README.md
Get-Content docs/testing.md
Get-Content docs/user_manual.md
Get-Content docs/PROJECT_PROGRESS.md
```

预期：
- 各文档中的构建命令保持一致。
- 对夹具准备流程和 Windows Release AI 测试限制的解释一致。
- 不再保留依赖本地缺失资产的绝对性结论。

- [ ] **步骤 5：提交**

```bash
git add README.md docs/testing.md docs/user_manual.md docs/PROJECT_PROGRESS.md
git commit -m "文档：按可复现状态更新项目说明"
```

## 优先级顺序

1. 任务 1：让测试资产可复现
2. 任务 2：统一 CMake 行为与构建意图
3. 任务 3：消除 CI 歧义并确保测试真实执行
4. 任务 4：验证安装与外部消费链路
5. 任务 5：刷新项目状态与发布就绪度

## 验收标准

- 干净仓库不再依赖未文档化的本地文件。
- 构建预设与 CI 对“是否编译测试”达成一致。
- 团队可以直接从仓库内复现文档描述的验证路径。
- 安装/导出链路通过外部消费工程验证。
- 项目进度文档能够清楚区分“已实现”与“可复现验证”。
