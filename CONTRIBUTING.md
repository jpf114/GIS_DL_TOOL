# Contributing

## Local Workflow

```bash
cmake --preset release
cmake --build --preset release
```

Before submitting changes:

- keep edits focused
- update affected tests
- run the relevant `ctest` command for the modules you changed
- keep public C API examples aligned with `include/gis_ai/gis_ai.h`

## Notes

- Windows presets use `VCPKG_ROOT`
- ONNX overlay patches live in `vcpkg-overlay/onnx`
- Windows presets export `VCPKG_OVERLAY_PORTS=${sourceDir}/vcpkg-overlay`
- The ONNX overlay must keep `ONNX_DISABLE_STATIC_REGISTRATION=ON` for ONNX Runtime compatibility
- AI integration tests run in `Release` on Windows
- Keep non-business build/install/GUI behavior aligned with `D:\Code\MyProject\GIS_TOOL`
