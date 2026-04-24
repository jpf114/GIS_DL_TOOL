# GIS AI C API Reference

This document lists the currently exported public C API declared in `include/gis_ai/gis_ai.h`.

## Initialization and Errors

```c
int GisAi_Init(const char* config_path);
void GisAi_Shutdown(void);
const char* GisAi_GetLastError(void);
int GisAi_GetLastErrorCode(void);
```

- `GisAi_Init`: initializes logging and optionally loads a config file.
- `GisAi_GetLastError`: returns the last thread-local error message.

## Version

```c
int GisAi_GetVersionMajor(void);
int GisAi_GetVersionMinor(void);
int GisAi_GetVersionPatch(void);
const char* GisAi_GetVersionString(void);
```

## Raster

```c
GisAiRaster* GisAi_Raster_Load(const char* path);
int GisAi_Raster_Save(GisAiRaster* raster, const char* path);
void GisAi_Raster_Destroy(GisAiRaster* raster);
int GisAi_Raster_GetWidth(GisAiRaster* raster);
int GisAi_Raster_GetHeight(GisAiRaster* raster);
int GisAi_Raster_GetBandCount(GisAiRaster* raster);
int GisAi_Raster_GetGeoTransform(GisAiRaster* raster, double* out_transform);
const char* GisAi_Raster_GetProjection(GisAiRaster* raster);
int GisAi_Raster_GetBandData(GisAiRaster* raster, int band_index, float* out_buffer, int buffer_size);
```

## Vector

```c
GisAiVector* GisAi_Vector_Load(const char* path);
int GisAi_Vector_Save(GisAiVector* vector, const char* path);
void GisAi_Vector_Destroy(GisAiVector* vector);
int GisAi_Vector_GetFeatureCount(GisAiVector* vector);
const char* GisAi_Vector_GetProjection(GisAiVector* vector);
GisAiVector* GisAi_Vector_Buffer(GisAiVector* vector, double distance);
GisAiVector* GisAi_Vector_Intersect(GisAiVector* a, GisAiVector* b);
GisAiVector* GisAi_Vector_Clip(GisAiVector* target, GisAiVector* clipper);
GisAiVector* GisAi_Vector_Simplify(GisAiVector* vector, double tolerance);
```

## Point Cloud

```c
GisAiPointCloud* GisAi_PointCloud_Load(const char* path);
int GisAi_PointCloud_Save(GisAiPointCloud* pc, const char* path);
void GisAi_PointCloud_Destroy(GisAiPointCloud* pc);
int GisAi_PointCloud_GetPointCount(GisAiPointCloud* pc);
```

## Raster Algorithms

```c
GisAiRaster* GisAi_Raster_Resample(GisAiRaster* raster, int new_width, int new_height, const char* method);
GisAiRaster* GisAi_Raster_Normalize(GisAiRaster* raster);
GisAiRaster* GisAi_Raster_Threshold(GisAiRaster* raster, double threshold);
```

## Model and Inference

```c
GisAiModel* GisAi_Model_Load(const char* model_path);
void GisAi_Model_Unload(GisAiModel* model);
GisAiRaster* GisAi_AI_Infer(GisAiModel* model, GisAiRaster* input);
```

## Segmentation Pipeline

```c
GisAiRasterSeg* GisAi_RasterSeg_Create(const char* model_path);
int GisAi_RasterSeg_Run(GisAiRasterSeg* seg, const char* input_tif, const char* output_tif, const char* output_shp);
void GisAi_RasterSeg_Destroy(GisAiRasterSeg* seg);
```

- `output_shp` may be `NULL` if polygon output is not needed.

## Coordinate Transform

```c
int GisAi_TransformCoordinates(double* x, double* y, const char* from_crs, const char* to_crs);
```

## Ownership Rules

- Objects returned by `Load`, `Create`, and algorithm functions must be released by the matching `Destroy` or `Unload`.
- Strings returned by `GisAi_GetLastError`, `GisAi_Raster_GetProjection`, and `GisAi_Vector_GetProjection` are owned by the library and should not be freed by the caller.
