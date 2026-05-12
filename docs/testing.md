# Testing Guide

This document reflects the repository state verified on 2026-05-13.

## Scope

The current test story is strongest on Windows release builds. Debug and cross-platform paths still depend on local environment details and have not been re-verified in this current sweep.

## Prerequisites

- Windows
- Visual Studio 2022
- CMake 3.21+
- `VCPKG_ROOT` pointing at the shared global `vcpkg`

## Configure and Build

Release:

```powershell
cmake --preset release
cmake --build --preset release
```

Debug:

```powershell
cmake --preset debug
cmake --build --preset debug
```

## Focused Release Verification

These are the commands that match the current phase checks:

```powershell
ctest --test-dir build/release -C Release --output-on-failure -R test_gui_task_database
ctest --test-dir build/release -C Release --output-on-failure -R test_gui_queue
ctest --test-dir build/release -C Release --output-on-failure -R gui_smoke_test
ctest --test-dir build/release -C Release --output-on-failure -R test_io_integration
```

## What Each Test Covers

- `test_gui_task_database`: task persistence schema and stored task metadata
- `test_gui_queue`: queued execution flow, log clearing, and task-center result detail presentation
- `gui_smoke_test`: GUI bootstrap automation path, parameter injection, auto-execute, screenshot/status artifacts
- `test_io_integration`: representative IO integration behavior against generated test assets

## Installed Tree Smoke Checks

After installing the release tree:

```powershell
cmake --install build/release --config Release
install\bin\gis_ai_cli.exe help
install\bin\gis-ai-gui.exe --self-test
```

These commands were verified successfully on 2026-05-13.

## Notes About Runtime Packaging

- The install step currently succeeds even when `dumpbin` or `objdump` is not found.
- In that case, `install_runtime_deps.cmake` falls back to copying the minimal runtime DLL list directly.
- That fallback path was exercised successfully during the 2026-05-13 install verification.

## Known Gaps

- No fresh claim is made here about Linux or macOS verification.
- No fresh claim is made here about Windows Debug parity for all integration targets.
- AI integration coverage outside the currently exercised release path still depends on local model/runtime setup.

## Recommended Current-Phase Check Order

```powershell
cmake --preset release
cmake --build --preset release
ctest --test-dir build/release -C Release --output-on-failure -R test_gui_task_database
ctest --test-dir build/release -C Release --output-on-failure -R test_gui_queue
ctest --test-dir build/release -C Release --output-on-failure -R gui_smoke_test
cmake --install build/release --config Release
install\bin\gis-ai-gui.exe --self-test
```
