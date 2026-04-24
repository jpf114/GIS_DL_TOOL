# Contributing

## Local Workflow

```bash
cmake --preset=dev-windows
cmake --build --preset=dev
```

Before submitting changes:

- keep edits focused
- update affected tests
- run the relevant `ctest` command for the modules you changed
- keep public C API examples aligned with `include/gis_ai/gis_ai.h`

## Notes

- Windows presets use `VCPKG_ROOT`
- ONNX overlay patches live in `vcpkg-overlay/onnx`
- AI integration tests run in `Release` on Windows
