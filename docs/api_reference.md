# GIS AI C API 参考

本文档整理了当前在 `include/gis_ai/gis_ai.h` 中对外暴露的公共 C API。

## 初始化与错误处理

```c
int GisAi_Init(const char* config_path);
void GisAi_Shutdown(void);
const char* GisAi_GetLastError(void);
int GisAi_GetLastErrorCode(void);
```

- `GisAi_Init`：初始化日志系统，并在传入配置路径时尝试加载配置文件。
- `GisAi_Shutdown`：执行库级清理。
- `GisAi_GetLastError`：返回当前线程最近一次错误消息。
- `GisAi_GetLastErrorCode`：返回当前线程最近一次错误码。

## 版本信息

```c
int GisAi_GetVersionMajor(void);
int GisAi_GetVersionMinor(void);
int GisAi_GetVersionPatch(void);
const char* GisAi_GetVersionString(void);
```

## 栅格相关

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

## 矢量相关

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

## 点云相关

```c
GisAiPointCloud* GisAi_PointCloud_Load(const char* path);
int GisAi_PointCloud_Save(GisAiPointCloud* pc, const char* path);
void GisAi_PointCloud_Destroy(GisAiPointCloud* pc);
int GisAi_PointCloud_GetPointCount(GisAiPointCloud* pc);
```

## 栅格算法

```c
GisAiRaster* GisAi_Raster_Resample(GisAiRaster* raster, int new_width, int new_height, const char* method);
GisAiRaster* GisAi_Raster_Normalize(GisAiRaster* raster);
GisAiRaster* GisAi_Raster_Threshold(GisAiRaster* raster, double threshold);
```

## 模型与推理

```c
GisAiModel* GisAi_Model_Load(const char* model_path);
void GisAi_Model_Unload(GisAiModel* model);
GisAiRaster* GisAi_AI_Infer(GisAiModel* model, GisAiRaster* input);
```

## 分割流程

```c
GisAiRasterSeg* GisAi_RasterSeg_Create(const char* model_path);
int GisAi_RasterSeg_Run(GisAiRasterSeg* seg, const char* input_tif, const char* output_tif, const char* output_shp);
void GisAi_RasterSeg_Destroy(GisAiRasterSeg* seg);
```

- `output_shp` 允许为 `NULL`，表示只输出栅格结果，不生成矢量结果。

## 坐标转换

```c
int GisAi_TransformCoordinates(double* x, double* y, const char* from_crs, const char* to_crs);
```

## 所有权规则

- 所有通过 `Load`、`Create`、算法函数返回的对象，都必须由调用方使用匹配的 `Destroy` 或 `Unload` 释放。
- `GisAi_GetLastError`、`GisAi_Raster_GetProjection`、`GisAi_Vector_GetProjection` 返回的字符串由库内部维护，调用方不应手动释放。
