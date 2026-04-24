# 测试指南

在运行集成测试前，请先准备标准测试夹具目录结构。

## 准备测试夹具

### Windows

```powershell
powershell -ExecutionPolicy Bypass -File scripts/generate_test_data.ps1
```

### Linux 和 macOS

```bash
bash scripts/generate_test_data.sh
```

仓库会保留 `test_data` 目录说明文档，但生成后的栅格、矢量和模型夹具仍然属于本地产物，不直接提交。
当前脚本可自动生成栅格与矢量夹具，但 `test_data/models/test_seg_model.onnx` 仍需单独准备。

## Windows

### Debug

```powershell
$env:PROJ_LIB = (Resolve-Path "build/dev-windows/vcpkg_installed/x64-windows/share/proj").Path
ctest --test-dir build/dev-windows -C Debug --output-on-failure -E test_ai_integration
```

### Release AI 集成测试

```powershell
$env:PROJ_LIB = (Resolve-Path "build/dev-windows/vcpkg_installed/x64-windows/share/proj").Path
ctest --test-dir build/dev-windows -C Release --output-on-failure -R test_ai_integration
```

由于 ONNX 依赖链在 Windows `Debug` 模式下仍存在 schema 注册问题，`test_ai_integration` 当前仅在 `Release` 模式下执行。

## Linux 和 macOS

```bash
ctest --output-on-failure -E test_ai_integration
```

## 测试目标

- `test_gis_ai`：覆盖 core、IO、GIS、AI 工具代码和 fusion 数据流的单元测试
- `test_io_integration`：真实文件 I/O 集成测试
- `test_ai_integration`：ONNX 模型加载与端到端分割流程测试
