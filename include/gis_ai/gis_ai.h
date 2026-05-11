#ifndef GIS_AI_H
#define GIS_AI_H

#include "gis_ai/export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GisAiRaster GisAiRaster;
typedef struct GisAiVector GisAiVector;
typedef struct GisAiPointCloud GisAiPointCloud;
typedef struct GisAiModel GisAiModel;
typedef struct GisAiRasterSeg GisAiRasterSeg;
typedef struct GisAiLargeImageSeg GisAiLargeImageSeg;

GIS_AI_API int GisAi_Init(const char* config_path);
GIS_AI_API void GisAi_Shutdown();
GIS_AI_API const char* GisAi_GetLastError();
GIS_AI_API int GisAi_GetLastErrorCode();

GIS_AI_API int GisAi_GetVersionMajor();
GIS_AI_API int GisAi_GetVersionMinor();
GIS_AI_API int GisAi_GetVersionPatch();
GIS_AI_API const char* GisAi_GetVersionString();

GIS_AI_API GisAiRaster* GisAi_Raster_Load(const char* path);
GIS_AI_API int GisAi_Raster_Save(GisAiRaster* raster, const char* path);
GIS_AI_API void GisAi_Raster_Destroy(GisAiRaster* raster);
GIS_AI_API int GisAi_Raster_GetWidth(GisAiRaster* raster);
GIS_AI_API int GisAi_Raster_GetHeight(GisAiRaster* raster);
GIS_AI_API int GisAi_Raster_GetBandCount(GisAiRaster* raster);
GIS_AI_API int GisAi_Raster_GetGeoTransform(GisAiRaster* raster, double* out_transform);
GIS_AI_API const char* GisAi_Raster_GetProjection(GisAiRaster* raster);
GIS_AI_API int GisAi_Raster_GetBandData(GisAiRaster* raster, int band_index, float* out_buffer, int buffer_size);

GIS_AI_API GisAiVector* GisAi_Vector_Load(const char* path);
GIS_AI_API int GisAi_Vector_Save(GisAiVector* vector, const char* path);
GIS_AI_API void GisAi_Vector_Destroy(GisAiVector* vector);
GIS_AI_API int GisAi_Vector_GetFeatureCount(GisAiVector* vector);
GIS_AI_API const char* GisAi_Vector_GetProjection(GisAiVector* vector);

GIS_AI_API GisAiPointCloud* GisAi_PointCloud_Load(const char* path);
GIS_AI_API int GisAi_PointCloud_Save(GisAiPointCloud* pc, const char* path);
GIS_AI_API void GisAi_PointCloud_Destroy(GisAiPointCloud* pc);
GIS_AI_API int GisAi_PointCloud_GetPointCount(GisAiPointCloud* pc);

GIS_AI_API GisAiVector* GisAi_Vector_Buffer(GisAiVector* vector, double distance);
GIS_AI_API GisAiVector* GisAi_Vector_Intersect(GisAiVector* a, GisAiVector* b);
GIS_AI_API GisAiVector* GisAi_Vector_Clip(GisAiVector* target, GisAiVector* clipper);
GIS_AI_API GisAiVector* GisAi_Vector_Simplify(GisAiVector* vector, double tolerance);

GIS_AI_API GisAiRaster* GisAi_Raster_Resample(GisAiRaster* raster, int new_width, int new_height, const char* method);
GIS_AI_API GisAiRaster* GisAi_Raster_Normalize(GisAiRaster* raster);
GIS_AI_API GisAiRaster* GisAi_Raster_Threshold(GisAiRaster* raster, double threshold);

GIS_AI_API GisAiModel* GisAi_Model_Load(const char* model_path);
GIS_AI_API void GisAi_Model_Unload(GisAiModel* model);
GIS_AI_API GisAiRaster* GisAi_AI_Infer(GisAiModel* model, GisAiRaster* input);

GIS_AI_API GisAiRasterSeg* GisAi_RasterSeg_Create(const char* model_path);
GIS_AI_API int GisAi_RasterSeg_Run(GisAiRasterSeg* seg, const char* input_tif,
                                    const char* output_tif, const char* output_shp);
GIS_AI_API void GisAi_RasterSeg_Destroy(GisAiRasterSeg* seg);

GIS_AI_API int GisAi_TransformCoordinates(double* x, double* y, const char* from_crs, const char* to_crs);

GIS_AI_API GisAiLargeImageSeg* GisAi_LargeImageSeg_Create(const char* model_path);
GIS_AI_API int GisAi_LargeImageSeg_Run(GisAiLargeImageSeg* seg, const char* input_tif,
                                         const char* output_tif, const char* output_shp,
                                         int tile_size, int stride, int blend_mode);
GIS_AI_API void GisAi_LargeImageSeg_Destroy(GisAiLargeImageSeg* seg);

#ifdef __cplusplus
}
#endif

#endif // GIS_AI_H
