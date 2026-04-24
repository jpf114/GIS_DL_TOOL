# User Manual

## Typical Segmentation Flow

1. Build the project and generate the shared library or executable tools.
2. Call `GisAi_Init`.
3. Create a segmentation object with `GisAi_RasterSeg_Create`.
4. Run segmentation with `GisAi_RasterSeg_Run`.
5. Destroy returned objects and call `GisAi_Shutdown`.

## Input and Output

- Raster input: GeoTIFF
- Vector output: Shapefile or GeoJSON
- Point cloud save output: Shapefile or GeoJSON point features

## Common Notes

- Coordinate transformation depends on PROJ data being available.
- Windows AI integration is expected to run in `Release`.
- Objects returned by `Load`, `Create`, and algorithm APIs must be released with the matching destroy function.
