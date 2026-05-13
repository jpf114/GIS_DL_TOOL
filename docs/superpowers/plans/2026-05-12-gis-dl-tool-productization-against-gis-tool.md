# GIS_DL_TOOL Productization Against GIS_TOOL Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Align `GIS_DL_TOOL` with `GIS_TOOL` on Windows productization: build/install layout, minimal runtime packaging, GUI shell structure, task execution model, and verification/documentation flow.

**Architecture:** Treat `GIS_TOOL` as the platform motherbase and `GIS_DL_TOOL` as a business variant. Reuse the same build/install/runtime conventions and the same GUI shell patterns, while keeping AI segmentation-specific actions and parameter models inside the `GIS_DL_TOOL` business bridge. Stabilize engineering first, then converge GUI execution flow, then close testing and docs.

**Tech Stack:** C++17, CMake 3.21+, Visual Studio 2022, vcpkg, Qt6 Widgets/Svg/Sql, SQLite3, GDAL, PROJ, GEOS, ONNX Runtime, GoogleTest

---

## File Map

**Primary files to modify**
- `CMakeLists.txt`
- `CMakePresets.json`
- `cmake/gis_ai_runtime_dlls.cmake`
- `cmake/install_runtime_deps.cmake.in`
- `src/gui/CMakeLists.txt`
- `src/gui/main.cpp`
- `src/gui/mainwindow.h`
- `src/gui/mainwindow.cpp`
- `src/gui/task_manager.h`
- `src/gui/task_manager.cpp`
- `src/gui/task_database.h`
- `src/gui/task_database.cpp`
- `src/gui/task_center_page.cpp`
- `src/gui/gui_data_support.h`
- `src/gui/gui_data_support.cpp`
- `src/core/logger.h`
- `src/core/logger.cpp`
- `tests/CMakeLists.txt`
- `docs/testing.md`
- `README.md`
- `docs/PROJECT_PROGRESS.md`

**Likely new files**
- `src/gui/task_runner.h`
- `src/gui/task_runner.cpp`

**Reference-only motherbase files**
- `D:/Code/MyProject/GIS_TOOL/CMakeLists.txt`
- `D:/Code/MyProject/GIS_TOOL/CMakePresets.json`
- `D:/Code/MyProject/GIS_TOOL/cmake/gis_runtime_dlls.cmake`
- `D:/Code/MyProject/GIS_TOOL/cmake/install_runtime_deps.cmake.in`
- `D:/Code/MyProject/GIS_TOOL/src/gui/main.cpp`
- `D:/Code/MyProject/GIS_TOOL/src/gui/CMakeLists.txt`
- `D:/Code/MyProject/GIS_TOOL/src/gui/mainwindow.cpp`
- `D:/Code/MyProject/GIS_TOOL/src/gui/task_runner.cpp`
- `D:/Code/MyProject/GIS_TOOL/src/gui/task_database.cpp`
- `D:/Code/MyProject/GIS_TOOL/src/gui/gui_data_support.cpp`

---

### Task 1: Lock Build/Install Motherbase Parity

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `CMakePresets.json`
- Modify: `cmake/gis_ai_runtime_dlls.cmake`
- Modify: `cmake/install_runtime_deps.cmake.in`

- [ ] **Step 1: Diff the current top-level build/install flow against the GIS motherbase**

Compare:
- vcpkg root inference
- `build/debug` / `build/release` behavior
- install directory variables
- runtime dependency copy strategy
- Qt platform/sql driver install hooks

Run:

```powershell
Get-Content D:\Code\MyProject\GIS_TOOL\CMakeLists.txt
Get-Content D:\Code\MyProject\GIS_DL_TOOL\CMakeLists.txt
Get-Content D:\Code\MyProject\GIS_TOOL\cmake\install_runtime_deps.cmake.in
Get-Content D:\Code\MyProject\GIS_DL_TOOL\cmake\install_runtime_deps.cmake.in
```

Expected: a concrete list of missing install/runtime behaviors.

- [ ] **Step 2: Update top-level install path and runtime directory variables to match the motherbase shape**

Ensure `GIS_DL_TOOL` defines and uses:
- install prefix = `${sourceDir}/install`
- `GIS_AI_VCPKG_*` path family
- install destinations for `platforms`, `sqldrivers`, `icons`, `share/proj`, `share/gdal`

Target shape:

```cmake
set(GIS_AI_INSTALL_PLATFORM_DIR "${CMAKE_INSTALL_BINDIR}/platforms")
set(GIS_AI_INSTALL_SQLDRIVERS_DIR "${CMAKE_INSTALL_BINDIR}/sqldrivers")
```

- [ ] **Step 3: Upgrade install-time runtime dependency handling**

Bring `cmake/install_runtime_deps.cmake.in` to the same model as `GIS_TOOL`:
- try `file(GET_RUNTIME_DEPENDENCIES ...)`
- fall back to a curated DLL list if scanning is unavailable
- avoid copying already-installed DLLs back onto themselves

- [ ] **Step 4: Bring the runtime DLL helper to motherbase parity**

Expand `cmake/gis_ai_runtime_dlls.cmake` so it:
- clearly separates Release and Debug DLL sets
- copies only the minimal runtime set
- uses `copy_if_exists.cmake` for Debug-only DLLs

- [ ] **Step 5: Reconfigure both presets**

Run:

```powershell
cmake --preset debug
cmake --preset release
```

Expected: both configure successfully with `build/debug` and `build/release`.

- [ ] **Step 6: Rebuild Debug and Release**

Run:

```powershell
cmake --build --preset debug
cmake --build --preset release
```

Expected: CLI, library, GUI, tests, and tools build successfully.

- [ ] **Step 7: Validate install shape**

Run:

```powershell
cmake --install build/release --config Release
Get-ChildItem install -Recurse
```

Expected: `install/bin`, `install/share/proj`, `install/share/gdal`, `install/share/icons`, and Qt runtime folders exist with minimal runtime contents.

---

### Task 2: Align GUI Packaging Rules

**Files:**
- Modify: `src/gui/CMakeLists.txt`

- [ ] **Step 1: Compare GUI build packaging against the motherbase**

Focus on:
- target name and output structure
- runtime copy call
- `platforms/`
- `sqldrivers/`
- icon copy
- Release install of Qt platform DLLs and SQLite driver

Run:

```powershell
Get-Content D:\Code\MyProject\GIS_TOOL\src\gui\CMakeLists.txt
Get-Content D:\Code\MyProject\GIS_DL_TOOL\src\gui\CMakeLists.txt
```

Expected: exact missing packaging behaviors are identified.

- [ ] **Step 2: Add Qt SQLite driver packaging parity**

Mirror the motherbase pattern:

```cmake
set(GIS_AI_VCPKG_QT_SQLDRIVERS_DIR ".../plugins/sqldrivers")
set(GIS_AI_QT_SQLITE_DLL "$<$<CONFIG:Debug>:qsqlited.dll>$<$<NOT:$<CONFIG:Debug>>:qsqlite.dll>")
```

- [ ] **Step 3: Ensure GUI build output is runnable from build directory**

The build output should contain:
- `gis-ai-gui.exe`
- `platforms/`
- `sqldrivers/`
- runtime DLLs
- copied icons

- [ ] **Step 4: Verify GUI launch from build/release**

Run:

```powershell
build\release\bin\Release\gis-ai-gui.exe --self-test
```

Expected: process starts and exits cleanly in self-test mode.

---

### Task 3: Align GUI Bootstrap Entry

**Files:**
- Modify: `src/gui/main.cpp`

- [ ] **Step 1: Port the motherbase bootstrap behavior selectively**

Bring over:
- font initialization pattern
- self-test mode
- optional screenshot/status-file/auto-execute CLI hooks where they make sense
- stable GDAL/PROJ runtime initialization

Keep `GIS_DL_TOOL` business identity in app name and organization name.

- [ ] **Step 2: Implement minimal automation hooks**

Support at least:
- `--self-test`
- `--screenshot <path>`
- `--status-file <path>`
- `--select-plugin <name>`
- `--select-action <name>`
- `--set-param key=value`
- `--auto-execute`

This does not need full parity in one pass, but the interface should be compatible with future GUI regression automation.

- [ ] **Step 3: Verify bootstrap hooks**

Run:

```powershell
build\release\bin\Release\gis-ai-gui.exe --self-test
build\release\bin\Release\gis-ai-gui.exe --screenshot D:\Code\MyProject\GIS_DL_TOOL\tmp_gui.png --self-test
```

Expected: no startup crash; screenshot mode writes a file if the hook is enabled.

---

### Task 4: Introduce TaskRunner Queue Model

**Files:**
- Create: `src/gui/task_runner.h`
- Create: `src/gui/task_runner.cpp`
- Modify: `src/gui/CMakeLists.txt`
- Modify: `src/gui/mainwindow.h`
- Modify: `src/gui/mainwindow.cpp`

- [ ] **Step 1: Add a queue-based task runner modeled after GIS_TOOL**

Responsibilities:
- queue submitted GUI tasks
- start one task at a time
- own the `QThread + ExecuteWorker + QtProgressReporter` wiring
- emit `taskStarted`, `taskProgressChanged`, `taskLogMessage`, `taskFinished`, `queueChanged`

- [ ] **Step 2: Move task orchestration out of MainWindow**

`MainWindow` should stop owning the full execution lifecycle directly and instead:
- submit tasks to `TaskRunner`
- listen for task state changes
- update result summary/status bar

- [ ] **Step 3: Keep current business behavior intact**

Do not regress:
- rerun/edit task flows
- batch submission
- task center synchronization

- [ ] **Step 4: Verify queue behavior**

Manual verification:
- submit two tasks
- confirm second task queues rather than spawning an uncontrolled extra worker

Expected: a stable queue-based execution model close to the motherbase.

---

### Task 5: Strengthen Task Persistence Model

**Files:**
- Modify: `src/gui/task_manager.h`
- Modify: `src/gui/task_manager.cpp`
- Modify: `src/gui/task_database.h`
- Modify: `src/gui/task_database.cpp`
- Modify: `src/gui/task_center_page.cpp`

- [ ] **Step 1: Compare current task schema against the motherbase**

Important fields:
- plugin display name
- action display name
- localized result message
- raw result message
- output path
- start/end time
- duration

- [ ] **Step 2: Add missing result fields where needed**

Bring `GIS_DL_TOOL` task persistence up to the motherbase level so the UI can display:
- localized message
- raw message
- stable duration
- richer completion state

- [ ] **Step 3: Keep schema migration backward-compatible**

Use `PRAGMA table_info(...)` checks and conditional `ALTER TABLE` updates rather than destructive resets.

- [ ] **Step 4: Align task center display logic**

Improve:
- duration fallback behavior
- status handling
- rerun/edit availability
- log coloring and row updates

- [ ] **Step 5: Verify persistence behavior**

Manual verification:
- submit task
- restart GUI
- confirm task history reloads
- rerun a completed task

Expected: persistent task history remains stable and behavior matches the motherbase closely.

---

### Task 6: Expand GUI Data Support Into the Real Business Bridge

**Files:**
- Modify: `src/gui/gui_data_support.h`
- Modify: `src/gui/gui_data_support.cpp`

- [ ] **Step 1: Reframe gui_data_support as the GUI platform behavior center**

It should own:
- action display names
- parameter display names/descriptions
- visible/required parameter sets
- auto-output derivation
- dataset-derived autofill
- action-specific validation
- highlighted invalid parameter resolution

- [ ] **Step 2: Keep AI workflow differences isolated here**

The `GIS_DL_TOOL`-specific business layer should describe:
- segmentation
- batch segmentation
- model path
- tile size / stride / blend mode
- output tif / output shp
- AI-specific normalization and postprocessing controls if exposed

- [ ] **Step 3: Improve output derivation and validation**

Target behavior:
- infer output paths from input raster when reasonable
- validate model path, raster path, output directory, and segmentation-specific numeric ranges
- preserve motherbase-like UX states for “ready / fix required / running”

- [ ] **Step 4: Verify parameter UX**

Manual verification:
- select segmentation action
- set only input/model
- confirm output paths auto-fill
- confirm required-field highlighting and button enable state behave predictably

Expected: the GUI feels like the same platform, just with different business actions.

---

### Task 7: Make Logging Non-Blocking

**Files:**
- Modify: `src/core/logger.h`
- Modify: `src/core/logger.cpp`
- Modify: relevant tests that assume file logging always works

- [ ] **Step 1: Change logger initialization to degrade gracefully**

Current failure mode:
- file log sink throws
- tests and application logic fail early

Desired behavior:
- try file sink
- if file sink fails, keep console sink alive
- do not let logging infrastructure break core functionality by default

- [ ] **Step 2: Add explicit test-safe initialization behavior**

Possible pattern:

```cpp
void Initialize(const std::string& log_file = "", spdlog::level::level_enum level = spdlog::level::info);
```

or keep the existing signature but handle file-sink failure internally.

- [ ] **Step 3: Re-run the core test suite**

Run:

```powershell
ctest --test-dir build/release -C Release --output-on-failure -R test_gis_ai
```

Expected: tests no longer fail solely because a log file cannot be opened.

---

### Task 8: Stabilize Test Runtime Behavior

**Files:**
- Modify: `tests/CMakeLists.txt`
- Modify: selected tests only if needed
- Possibly modify: `src/gui/main.cpp` for GUI self-test hooks

- [ ] **Step 1: Fix Debug/Release runtime lookup mismatches**

Address:
- missing test executables in Debug registration
- missing runtime DLLs in Debug launch
- any incorrect working directories

- [ ] **Step 2: Keep AI integration policy explicit**

Retain the Windows rule:
- non-AI tests may run in Debug
- AI integration runs in Release if ONNX Debug remains unstable

- [ ] **Step 3: Verify test discovery**

Run:

```powershell
ctest -N -C Debug --test-dir build/debug
ctest -N -C Release --test-dir build/release
```

Expected: registered tests point to real executables.

- [ ] **Step 4: Verify representative test execution**

Run:

```powershell
ctest --test-dir build/debug -C Debug --output-on-failure -R test_gis_ai
ctest --test-dir build/release -C Release --output-on-failure -R test_io_integration
ctest --test-dir build/release -C Release --output-on-failure -R test_ai_integration
```

Expected: failures, if any, are now real functional failures rather than environment/logging/runtime-lookup failures.

---

### Task 9: Close Install and External Consumption Validation

**Files:**
- Modify: `README.md`
- Modify: `docs/testing.md`
- Modify: `docs/PROJECT_PROGRESS.md`
- Possibly modify: example/install docs if behavior changed

- [ ] **Step 1: Validate installed CLI and GUI**

Run:

```powershell
install\bin\gis_ai_cli.exe help
install\bin\gis-ai-gui.exe --self-test
```

Expected: installed binaries start correctly.

- [ ] **Step 2: Validate installed package consumption**

Use the installed-package example or a minimal external smoke project with:

```cmake
find_package(gis_ai CONFIG REQUIRED)
```

Expected: configure and link succeed.

- [ ] **Step 3: Update repository docs to match actual behavior**

Docs must reflect:
- Windows-only scope
- motherbase alignment with `GIS_TOOL`
- current presets
- current test commands
- install layout
- known Release-vs-Debug test caveats

- [ ] **Step 4: Reconcile project progress narrative**

Update progress docs so they describe:
- current state
- what is aligned with `GIS_TOOL`
- what remains open

---

### Task 10: Final Verification Pass

**Files:**
- No new source files required; verification only

- [ ] **Step 1: Full release verification**

Run:

```powershell
cmake --preset release
cmake --build --preset release
ctest --test-dir build/release -C Release --output-on-failure
cmake --install build/release --config Release
```

Expected: release build, tests, and install all complete successfully or produce only documented residual failures.

- [ ] **Step 2: Debug verification**

Run:

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --test-dir build/debug -C Debug --output-on-failure -E test_ai_integration
```

Expected: debug build and non-AI validation path are stable.

- [ ] **Step 3: GUI smoke verification**

Run:

```powershell
build\release\bin\Release\gis-ai-gui.exe --self-test
```

Expected: GUI self-test exits cleanly.

- [ ] **Step 4: Manual acceptance checklist**

Confirm:
- build/install layout matches motherbase intent
- GUI shell looks and behaves like `GIS_TOOL`
- business differences are limited to AI/GIS functionality
- docs match reality

---

## Execution Order

1. Task 1: Lock Build/Install Motherbase Parity
2. Task 2: Align GUI Packaging Rules
3. Task 3: Align GUI Bootstrap Entry
4. Task 4: Introduce TaskRunner Queue Model
5. Task 5: Strengthen Task Persistence Model
6. Task 6: Expand GUI Data Support Into the Real Business Bridge
7. Task 7: Make Logging Non-Blocking
8. Task 8: Stabilize Test Runtime Behavior
9. Task 9: Close Install and External Consumption Validation
10. Task 10: Final Verification Pass

## Notes

- Do not introduce new cross-platform abstractions in this cycle.
- Prefer copying or adapting motherbase patterns over inventing new local variants.
- Treat any divergence from `GIS_TOOL` as suspicious unless it is directly required by AI segmentation business behavior.
- If a motherbase pattern depends on plugin DLLs that `GIS_DL_TOOL` does not have, keep the runtime/install structure but adapt the concrete target list conservatively.
