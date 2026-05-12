# GIS AI TOOL

Windows-focused GIS + AI toolkit for raster segmentation workflows.

`GIS_DL_TOOL` is the AI/GIS business variant built on top of the product shell patterns proven in `D:\Code\MyProject\GIS_TOOL`. The motherbase remains the reference for GUI shell behavior, runtime packaging, and productization conventions. This repository keeps the segmentation workflow, CLI/C API, and Qt desktop shell together while the remaining productization gaps are closed phase by phase.

## Current Position

- Primary platform: Windows + Visual Studio 2022
- Core workflow: ONNX-based raster segmentation and batch segmentation
- Delivery surfaces: CLI, C API, Qt GUI
- Motherbase relationship: `GIS_TOOL` is the platform motherbase; this repo selectively ports shared product behavior while keeping AI business logic local

## What Works Today

- Single-image raster segmentation
- Batch segmentation
- ONNX Runtime inference pipeline
- Raster/vector/point-cloud IO foundation
- Qt GUI with task queue, task persistence, progress, logs, rerun/edit flow, and structured result details
- Windows release build, install tree generation, and installed binary smoke verification

## What Is Still In Motion

- Ongoing motherbase alignment outside the segmentation-specific surface
- Wider install/export validation beyond the current Windows verification path
- Broader automated coverage for non-GUI productization scenarios
- Documentation cleanup for older claims that predate the current verification sweep

## Tech Stack

| Component | Version / Notes |
| --- | --- |
| Language | C++17 |
| Build system | CMake 3.21+ |
| Toolchain | Visual Studio 2022 |
| Package manager | vcpkg |
| GIS deps | GDAL, GEOS, PROJ |
| AI inference | ONNX Runtime |
| GUI | Qt6 Widgets/Svg/Sql |
| Local persistence | SQLite3 |
| Logging | spdlog |
| Tests | GoogleTest + CTest |

## Build

Prerequisites:

- Windows
- Visual Studio 2022
- CMake 3.21+
- global `vcpkg` installation with `VCPKG_ROOT` set

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

Install the release tree:

```powershell
cmake --install build/release --config Release
```

## Run

CLI help:

```powershell
build\release\bin\Release\gis_ai_cli.exe help
```

GUI:

```powershell
build\release\bin\Release\gis-ai-gui.exe
```

Installed binaries:

```powershell
install\bin\gis_ai_cli.exe help
install\bin\gis-ai-gui.exe --self-test
```

## Test

Focused release checks currently used in this phase:

```powershell
ctest --test-dir build/release -C Release --output-on-failure -R test_gui_task_database
ctest --test-dir build/release -C Release --output-on-failure -R test_gui_queue
ctest --test-dir build/release -C Release --output-on-failure -R gui_smoke_test
ctest --test-dir build/release -C Release --output-on-failure -R test_io_integration
```

More detail lives in [docs/testing.md](docs/testing.md).

## Repository Layout

```text
GIS_DL_TOOL/
|- include/gis_ai/          public headers
|- src/core/                core utilities
|- src/io/                  raster/vector/point-cloud IO
|- src/gis/                 GIS algorithms
|- src/ai/                  model loading and inference
|- src/fusion/              segmentation workflow layer
|- src/gui/                 Qt desktop shell
|- src/cli/                 command-line entry
|- tests/                   unit and integration tests
|- tools/                   helper executables
`- docs/                    project docs and plans
```

## Verified Snapshot

As of 2026-05-13, the following were verified on this machine:

- `cmake --preset release`
- `cmake --build --preset release`
- `ctest --test-dir build/release -C Release -R test_gui_queue`
- `ctest --test-dir build/release -C Release -R gui_smoke_test`
- `cmake --install build/release --config Release`
- `install\bin\gis_ai_cli.exe help`
- `install\bin\gis-ai-gui.exe --self-test`

## Docs

- [Testing guide](docs/testing.md)
- [Project progress](docs/PROJECT_PROGRESS.md)
- [Contributing](CONTRIBUTING.md)
