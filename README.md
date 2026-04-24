# GIS AI Algorithm Library

[![Build Status](https://github.com/your-org/gis_ai_lib/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/gis_ai_lib/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A C++17 GIS algorithm library focused on raster/vector/point-cloud processing and ONNX-based raster segmentation.

## Features

- Remote sensing raster segmentation based on ONNX Runtime
- Traditional GIS algorithms for vector, raster, and point cloud data
- Pure C API for C, C#, Python, Java, and other language bindings
- Cross-platform build presets for Windows, Linux, and macOS

## Technology Stack

| Component | Technology | Version |
|-----------|------------|---------|
| Language | C++ | C++17 |
| Build System | CMake | 3.16+ |
| Package Manager | vcpkg | latest |
| GIS Libraries | GDAL/GEOS/PROJ | 3.6+/3.11+/9.2+ |
| AI Inference | ONNX Runtime | 1.16+ |
| Logging | spdlog | 1.12+ |
| Testing | GoogleTest | 1.14+ |

## Quick Start

### Prerequisites

- CMake 3.16 or higher
- C++17 compatible compiler
- vcpkg

### Build

```bash
cmake --preset=dev-windows
cmake --build --preset=dev
```

### Test

Windows debug tests that use PROJ need `PROJ_LIB` to point to the vcpkg PROJ data directory:

```powershell
$env:PROJ_LIB = (Resolve-Path "build/dev-windows/vcpkg_installed/x64-windows/share/proj").Path
ctest --test-dir build/dev-windows -C Debug --output-on-failure -E test_ai_integration
ctest --test-dir build/dev-windows -C Release --output-on-failure -R test_ai_integration
```

`test_ai_integration` currently runs in `Release` because the ONNX dependency chain still has a Windows `Debug` schema registration issue.

## Project Structure

```text
gis_ai_lib/
|-- CMakeLists.txt
|-- CMakePresets.json
|-- vcpkg.json
|-- include/gis_ai/
|-- src/
|   |-- core/
|   |-- io/
|   |-- gis/
|   |-- ai/
|   `-- fusion/
|-- tests/
|-- examples/
|-- scripts/
|-- docker/
`-- docs/
```

## Module Overview

### Core

- Logging, exception, platform, memory, and config helpers

### IO

- Raster I/O: TIFF
- Vector I/O: Shapefile, GeoJSON
- Point cloud I/O: point-feature based read/write through GDAL/OGR

### GIS

- Vector: buffer, intersection, clip, simplify, topology check
- Raster: resample, normalize, clip, threshold, mosaic
- Point cloud: filter, downsampling
- Coordinate transform: PROJ-based CRS conversion

### AI

- ONNX Runtime integration
- Model management
- Raster preprocessing and postprocessing
- Mask to polygon conversion

### Fusion

- End-to-end raster segmentation
- Batch file processing

## C API Example

```cpp
#include <gis_ai/gis_ai.h>

int main() {
    if (GisAi_Init(nullptr) != 0) {
        return 1;
    }

    GisAiRasterSeg* seg = GisAi_RasterSeg_Create("test_data/models/test_seg_model.onnx");
    if (!seg) {
        GisAi_Shutdown();
        return 1;
    }

    int ret = GisAi_RasterSeg_Run(
        seg,
        "test_data/raster/test_100x100.tif",
        "test_data/raster/test_seg_output.tif",
        "test_data/vector/test_seg_output.shp");

    GisAi_RasterSeg_Destroy(seg);
    GisAi_Shutdown();
    return ret;
}
```

## Documentation

- [Project Progress](docs/PROJECT_PROGRESS.md)
- [Architecture Design](docs/architecture.md)
- [API Reference](docs/api_reference.md)

## Development Setup

```bash
docker build -t gis_ai_dev -f docker/Dockerfile.dev .
docker run -it gis_ai_dev bash
```

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
