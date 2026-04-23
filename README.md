# GIS AI Algorithm Library

[![Build Status](https://github.com/your-org/gis_ai_lib/actions/workflows/build.yml/badge.svg)](https://github.com/your-org/gis_ai_lib/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A high-performance, cross-platform C++ GIS AI algorithm library with no Python dependency.

## Features

- **Remote Sensing Image AI Segmentation**: Building, road, water, vegetation detection
- **LiDAR Point Cloud AI Classification**: Ground, building, vegetation, power line classification  
- **Traditional GIS Algorithms**: Vector/raster/point cloud operations
- **Pure C API**: Compatible with C#, Python, Java and other languages
- **Cross-platform**: Windows (MSVC), Linux (GCC), macOS (Clang)

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
- C++17 compatible compiler (MSVC 2019+, GCC 9+, Clang 10+)
- vcpkg (for dependency management)
- Ninja build tool (recommended)

### Installation

```bash
# Clone the repository
git clone https://github.com/your-org/gis_ai_lib.git
cd gis_ai_lib

# Install dependencies via vcpkg
export VCPKG_ROOT=/path/to/vcpkg  # Set vcpkg root path

# Configure with CMake (Windows Development example)
cmake --preset=dev-windows

# Build
cmake --build --preset=dev

# Run tests
ctest --test-dir build/dev-windows
```

### Platform-specific Presets

| Platform | Development | Release |
|----------|-------------|---------|
| Windows | `dev-windows` | `release-windows` |
| Linux | `dev-linux` | `release-linux` |
| macOS | `dev-macos` | `release-macos` |

## Project Structure

```
gis_ai_lib/
├── CMakeLists.txt          # Root build configuration
├── CMakePresets.json       # CMake presets for all platforms
├── vcpkg.json              # Dependency manifest
├── include/
│   ├── gis_ai/             # Public headers
│   └── internal/           # Internal headers
├── src/
│   ├── core/               # Core utilities (logging, exception, memory)
│   ├── io/                 # Data I/O (raster, vector, point cloud)
│   ├── gis/                # Traditional GIS algorithms
│   ├── ai/                 # AI inference module
│   └── fusion/             # AI+GIS fusion algorithms
├── models/                 # ONNX model storage
├── tests/                  # Unit tests
├── examples/               # Example programs
├── scripts/                # Utility scripts
├── docker/                 # Docker development environment
└── docs/                   # Documentation
```

## Module Overview

### Core Module (`src/core/`)
- Logging system (spdlog wrapper)
- Exception handling (GisAiException)
- Cross-platform abstraction
- Memory management utilities
- Configuration management

### IO Module (`src/io/`)
- Raster I/O: TIFF format (single/multi-band)
- Vector I/O: Shapefile, GeoJSON
- Point Cloud I/O: LAS format

### GIS Module (`src/gis/`)
- **Vector**: Buffer, intersection, clip, simplify, topology check
- **Raster**: Resample, normalize, clip, threshold, mosaic
- **Point Cloud**: Filter, downsampling
- **Coordinate**: Projection transformation (PROJ)

### AI Module (`src/ai/`)
- ONNX Runtime integration
- Model management
- Image preprocessing/postprocessing
- Mask to polygon conversion

### Fusion Module (`src/fusion/`)
- RasterSeg: End-to-end remote sensing segmentation
- BatchProcessor: Batch inference support
- Configuration system

## Usage Example

```cpp
#include <gis_ai/gis_ai.h>

int main() {
    // Initialize library
    GisAi_Init();
    
    // Load raster data
    GisAi_Raster* raster = GisAi_Raster_Load("input.tif");
    
    // Perform AI segmentation
    GisAi_RasterSeg* segmenter = GisAi_RasterSeg_Create("models/mobilesam.onnx");
    GisAi_Raster* result = GisAi_RasterSeg_Run(segmenter, raster);
    
    // Save result
    GisAi_Raster_Save(result, "output.tif");
    
    // Export to vector
    GisAi_Vector* vector = GisAi_MaskToPolygon(result);
    GisAi_Vector_Save(vector, "output.shp");
    
    // Cleanup
    GisAi_RasterSeg_Destroy(segmenter);
    GisAi_Raster_Destroy(raster);
    GisAi_Raster_Destroy(result);
    GisAi_Vector_Destroy(vector);
    GisAi_Shutdown();
    
    return 0;
}
```

## Development Roadmap

| Phase | Description | Timeline |
|-------|-------------|----------|
| Phase 0 | Project infrastructure | Week 1-2 |
| Phase 1 | Core infrastructure (core/io) | Week 3-5 |
| Phase 2 | Traditional GIS algorithms | Week 6-8 |
| Phase 3 | AI inference module | Week 9-10 |
| Phase 4 | AI+GIS fusion | Week 11-12 |
| Phase 5 | Final release | Week 13-14 |

See [docs/PROJECT_PROGRESS.md](docs/PROJECT_PROGRESS.md) for detailed progress tracking.

## Documentation

- [Project Progress](docs/PROJECT_PROGRESS.md) - Detailed progress tracking
- [Architecture Design](docs/architecture.md) - Architecture documentation (WIP)
- [API Reference](docs/api_reference.md) - API documentation (WIP)

## Contributing

We welcome contributions! Please see our contributing guidelines for details.

### Development Setup

```bash
# Using Docker for consistent environment
docker build -t gis_ai_dev -f docker/Dockerfile.dev .
docker run -it gis_ai_dev bash
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- GDAL - Geospatial Data Abstraction Library
- GEOS - Geometry Engine, Open Source
- PROJ - Cartographic Projections and Coordinate Transformations
- ONNX Runtime - Cross-platform inference engine
- spdlog - Fast C++ logging library
- GoogleTest - C++ testing framework

---

*This project is under active development. Some features are still in implementation.*