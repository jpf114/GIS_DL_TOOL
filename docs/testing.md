# Testing Guide

## Windows

### Debug

```powershell
$env:PROJ_LIB = (Resolve-Path "build/dev-windows/vcpkg_installed/x64-windows/share/proj").Path
ctest --test-dir build/dev-windows -C Debug --output-on-failure -E test_ai_integration
```

### Release AI integration

```powershell
$env:PROJ_LIB = (Resolve-Path "build/dev-windows/vcpkg_installed/x64-windows/share/proj").Path
ctest --test-dir build/dev-windows -C Release --output-on-failure -R test_ai_integration
```

`test_ai_integration` is limited to `Release` on Windows because the ONNX dependency chain still has a `Debug` schema registration issue.

## Linux and macOS

```bash
ctest --output-on-failure -E test_ai_integration
```

## Test Targets

- `test_gis_ai`: unit tests for core, IO, GIS, AI utility code, and fusion data flow
- `test_io_integration`: real file I/O integration tests
- `test_ai_integration`: ONNX model loading and end-to-end segmentation tests
