# GIS_DL_TOOL Current Phase Execution Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Finish the current productization phase by closing the highest-value gaps in GUI task-center behavior, Windows build/install parity, and repository documentation.

**Architecture:** Keep `D:\Code\MyProject\GIS_TOOL` as the product motherbase and treat `GIS_DL_TOOL` as the AI/GIS business variant. Continue converging shared platform behavior in the GUI shell, task persistence, runtime packaging, and verification flow, while keeping segmentation-specific differences inside `src/gui/gui_data_support.cpp` and the fusion/business layer.

**Tech Stack:** C++17, CMake 3.21+, Visual Studio 2022, vcpkg, Qt6 Widgets/Svg/Sql, SQLite3, GDAL, PROJ, GEOS, ONNX Runtime, GoogleTest

---

## Current Status Snapshot

- Already landed:
  - queue-based GUI submission path via `src/gui/task_runner.cpp`
  - non-blocking single-task execution in `src/gui/mainwindow.cpp`
  - task persistence now stores `resultMessage`, `resultRawMessage`, `outputPath`, and `durationMs`
  - task-log clearing now really clears database rows
  - GUI regression coverage in `tests/test_gui_queue.cpp`
  - task persistence regression coverage in `tests/test_gui_task_database.cpp`
- Verified recently:
  - `ctest --test-dir build/release -C Release -R test_gui_queue`
  - `ctest --test-dir build/release -C Release -R test_gui_task_database`
  - `ctest --test-dir build/release -C Release -R gui_smoke_test`
- Still open:
  - task-center result presentation is weaker than the motherbase
  - GUI automation/bootstrap parity is incomplete
  - install/runtime parity still needs a full motherbase pass
  - README/testing/progress docs still overstate maturity in places

## File Map

**Primary source files**
- `src/gui/main.cpp`
- `src/gui/mainwindow.h`
- `src/gui/mainwindow.cpp`
- `src/gui/task_center_page.h`
- `src/gui/task_center_page.cpp`
- `src/gui/task_manager.h`
- `src/gui/task_manager.cpp`
- `src/gui/task_database.h`
- `src/gui/task_database.cpp`
- `src/gui/gui_data_support.cpp`
- `src/gui/CMakeLists.txt`
- `CMakeLists.txt`
- `CMakePresets.json`
- `cmake/gis_ai_runtime_dlls.cmake`
- `cmake/install_runtime_deps.cmake.in`

**Primary test files**
- `tests/CMakeLists.txt`
- `tests/test_gui_queue.cpp`
- `tests/test_gui_task_database.cpp`

**Primary docs**
- `README.md`
- `docs/testing.md`
- `docs/PROJECT_PROGRESS.md`
- `docs/实用化功能路线图.md`

**Motherbase references**
- `D:/Code/MyProject/GIS_TOOL/src/gui/main.cpp`
- `D:/Code/MyProject/GIS_TOOL/src/gui/mainwindow.cpp`
- `D:/Code/MyProject/GIS_TOOL/src/gui/task_center_page.cpp`
- `D:/Code/MyProject/GIS_TOOL/src/gui/task_database.cpp`
- `D:/Code/MyProject/GIS_TOOL/src/gui/gui_data_support.cpp`
- `D:/Code/MyProject/GIS_TOOL/src/gui/CMakeLists.txt`
- `D:/Code/MyProject/GIS_TOOL/CMakeLists.txt`

---

### Task 1: Finish Task Center Result Presentation

**Files:**
- Modify: `src/gui/task_center_page.h`
- Modify: `src/gui/task_center_page.cpp`
- Modify: `src/gui/mainwindow.cpp`
- Modify: `tests/test_gui_queue.cpp`

- [ ] **Step 1: Write the failing GUI regression for result visibility**

Add a focused test to `tests/test_gui_queue.cpp` that creates a finished task with:
- `resultMessage = "执行失败"`
- `resultRawMessage = "onnxruntime: invalid model path"`
- `outputPath = "D:/tmp/out.tif"`

The test should select the task row and assert that the details area exposes enough information for a user to distinguish localized status from raw cause.

- [ ] **Step 2: Run the GUI regression to verify it fails**

Run:

```powershell
cmake --build --preset release --target test_gui_queue
ctest --test-dir build/release -C Release --output-on-failure -R test_gui_queue
```

Expected: FAIL because the current task-center/log/detail presentation does not surface the richer result data clearly enough.

- [ ] **Step 3: Implement the minimal task-center display upgrade**

Update `src/gui/task_center_page.cpp` so that selecting a completed or failed task shows:
- localized result summary
- raw result message when it differs
- output path when present
- duration in a stable format

Keep the UI structure consistent with the current page rather than inventing a separate modal.

- [ ] **Step 4: Re-run the GUI regression**

Run:

```powershell
ctest --test-dir build/release -C Release --output-on-failure -R test_gui_queue
```

Expected: PASS with the new result-detail behavior covered.

- [ ] **Step 5: Commit**

```powershell
git add src/gui/task_center_page.h src/gui/task_center_page.cpp src/gui/mainwindow.cpp tests/test_gui_queue.cpp
git commit -m "feat: improve task center result visibility"
```

---

### Task 2: Complete GUI Bootstrap and Automation Parity

**Files:**
- Modify: `src/gui/main.cpp`
- Modify: `src/gui/mainwindow.h`
- Modify: `src/gui/mainwindow.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write a failing bootstrap verification target**

Extend the existing GUI smoke path so it can assert support for:
- `--self-test`
- `--select-plugin`
- `--select-action`
- `--set-param key=value`
- `--auto-execute`

Do not introduce a new framework. Reuse the current GUI smoke/self-test path and only add the missing assertions.

- [ ] **Step 2: Run the smoke verification to confirm the missing hooks**

Run:

```powershell
ctest --test-dir build/release -C Release --output-on-failure -R gui_smoke_test
```

Expected: FAIL or incomplete coverage showing that current bootstrap automation is still weaker than the motherbase.

- [ ] **Step 3: Port the missing bootstrap behavior from the motherbase selectively**

Update `src/gui/main.cpp` to support the automation hooks needed for future regression runs while preserving `GIS_DL_TOOL` naming and business behavior.

At minimum, ensure the startup path can:
- select plugin/action
- apply parameter overrides
- trigger execution
- write status output deterministically in self-test automation flows

- [ ] **Step 4: Re-run the smoke verification**

Run:

```powershell
ctest --test-dir build/release -C Release --output-on-failure -R gui_smoke_test
```

Expected: PASS with the upgraded bootstrap path.

- [ ] **Step 5: Commit**

```powershell
git add src/gui/main.cpp src/gui/mainwindow.h src/gui/mainwindow.cpp tests/CMakeLists.txt
git commit -m "feat: align gui bootstrap automation hooks"
```

---

### Task 3: Close Windows Runtime and Install Parity

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `CMakePresets.json`
- Modify: `cmake/gis_ai_runtime_dlls.cmake`
- Modify: `cmake/install_runtime_deps.cmake.in`
- Modify: `src/gui/CMakeLists.txt`

- [ ] **Step 1: Diff current packaging against the motherbase**

Compare the following areas against `D:\Code\MyProject\GIS_TOOL`:
- runtime DLL copy policy
- `platforms/` packaging
- `sqldrivers/` packaging
- install directory structure
- `build/debug` and `build/release` preset expectations

- [ ] **Step 2: Write down the exact missing runtime/install behaviors**

Capture concrete deltas before editing:
- missing Qt SQLite driver copy/install
- missing fallback runtime scan behavior
- path mismatches between build tree and install tree

This note lives in the commit message and working notes; do not leave the repo in a speculative partial state.

- [ ] **Step 3: Implement the minimal packaging parity changes**

Bring `GIS_DL_TOOL` to the same practical layout as the motherbase for:
- `install/bin`
- `install/bin/platforms`
- `install/bin/sqldrivers`
- `install/share/proj`
- `install/share/gdal`
- `install/share/icons`

- [ ] **Step 4: Reconfigure, rebuild, and install**

Run:

```powershell
cmake --preset release
cmake --build --preset release
cmake --install build/release --config Release
```

Expected: configure/build/install all succeed.

- [ ] **Step 5: Verify installed binaries**

Run:

```powershell
install\bin\gis_ai_cli.exe help
install\bin\gis-ai-gui.exe --self-test
```

Expected: both binaries start correctly from `install/bin`.

- [ ] **Step 6: Commit**

```powershell
git add CMakeLists.txt CMakePresets.json cmake/gis_ai_runtime_dlls.cmake cmake/install_runtime_deps.cmake.in src/gui/CMakeLists.txt
git commit -m "build: align windows runtime and install packaging"
```

---

### Task 4: Refresh Docs to Match Reality

**Files:**
- Modify: `README.md`
- Modify: `docs/testing.md`
- Modify: `docs/PROJECT_PROGRESS.md`

- [ ] **Step 1: Identify every overstated or stale claim**

Specifically check:
- build commands
- test commands
- Windows-only scope
- install behavior
- GUI maturity claims
- motherbase alignment status

- [ ] **Step 2: Rewrite README around the real product position**

`README.md` should clearly say:
- this is a Windows-focused GIS + AI tool
- `GIS_TOOL` is the platform motherbase
- current strengths are segmentation workflow, GUI shell, CLI/C API, and productization progress
- known gaps remain in ongoing productization

- [ ] **Step 3: Rewrite testing and progress docs around verified reality**

`docs/testing.md` and `docs/PROJECT_PROGRESS.md` should describe:
- what is actually verified
- what still depends on environment setup
- what still remains open in the current phase

- [ ] **Step 4: Review the docs against current commands**

Run:

```powershell
Get-Content README.md
Get-Content docs/testing.md
Get-Content docs/PROJECT_PROGRESS.md
```

Expected: the three docs tell the same story and match the current build/test flow.

- [ ] **Step 5: Commit**

```powershell
git add README.md docs/testing.md docs/PROJECT_PROGRESS.md
git commit -m "docs: align productization docs with current project state"
```

---

### Task 5: Final Current-Phase Verification

**Files:**
- Verification only

- [ ] **Step 1: Rebuild the current release tree**

Run:

```powershell
cmake --preset release
cmake --build --preset release
```

Expected: successful release configure/build.

- [ ] **Step 2: Run focused GUI and persistence coverage**

Run:

```powershell
ctest --test-dir build/release -C Release --output-on-failure -R test_gui_task_database
ctest --test-dir build/release -C Release --output-on-failure -R test_gui_queue
ctest --test-dir build/release -C Release --output-on-failure -R gui_smoke_test
```

Expected: PASS for all three.

- [ ] **Step 3: Run representative non-GUI verification**

Run:

```powershell
ctest --test-dir build/release -C Release --output-on-failure -R test_io_integration
```

Expected: either PASS or a real functional/environment failure that can be documented explicitly.

- [ ] **Step 4: Install and smoke the installed package**

Run:

```powershell
cmake --install build/release --config Release
install\bin\gis-ai-gui.exe --self-test
```

Expected: install tree remains runnable after the current-phase changes.

- [ ] **Step 5: Commit phase closure if no new fixes are needed**

```powershell
git status --short
```

Expected: clean working tree or only intentional follow-up changes.

---

## Execution Order

1. Task 1: Finish Task Center Result Presentation
2. Task 2: Complete GUI Bootstrap and Automation Parity
3. Task 3: Close Windows Runtime and Install Parity
4. Task 4: Refresh Docs to Match Reality
5. Task 5: Final Current-Phase Verification

## Why This Plan Exists Alongside the Broader One

`docs/superpowers/plans/2026-05-12-gis-dl-tool-productization-against-gis-tool.md` remains the wider motherbase-alignment plan.

This document is the narrower current-phase execution plan. It exists so the active work can stay focused on:
- finishing the GUI/task-center productization slice already in motion
- making install/runtime behavior believable
- making repository docs tell the truth
