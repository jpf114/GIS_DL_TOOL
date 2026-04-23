#ifndef GIS_AI_H
#define GIS_AI_H

#ifdef __cplusplus
extern "C" {
#endif

// Export macro for shared library
#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef GIS_AI_SHARED
        #define GIS_AI_API __declspec(dllexport)
    #else
        #define GIS_AI_API __declspec(dllimport)
    #endif
#else
    #define GIS_AI_API __attribute__((visibility("default")))
#endif

// ============================================================================
// Core Types
// ============================================================================

typedef struct GisAiRaster GisAiRaster;
typedef struct GisAiVector GisAiVector;
typedef struct GisAiPointCloud GisAiPointCloud;
typedef struct GisAiModel GisAiModel;
typedef struct GisAiRasterSeg GisAiRasterSeg;

// ============================================================================
// Initialization and Shutdown
// ============================================================================

/**
 * Initialize the GIS AI library.
 * @param config_path Path to configuration file (can be NULL for defaults)
 * @return 0 on success, negative error code on failure
 */
GIS_AI_API int GisAi_Init(const char* config_path);

/**
 * Shutdown the GIS AI library and release all resources.
 */
GIS_AI_API void GisAi_Shutdown();

/**
 * Get the last error message.
 * @return Error message string (do not free)
 */
GIS_AI_API const char* GisAi_GetLastError();

/**
 * Get the last error code.
 * @return Error code
 */
GIS_AI_API int GisAi_GetLastErrorCode();

// ============================================================================
// Raster I/O
// ============================================================================

GIS_AI_API GisAiRaster* GisAi_Raster_Load(const char* path);
GIS_AI_API int GisAi_Raster_Save(GisAiRaster* raster, const char* path);
GIS_AI_API void GisAi_Raster_Destroy(GisAiRaster* raster);
GIS_AI_API int GisAi_Raster_GetWidth(GisAiRaster* raster);
GIS_AI_API int GisAi_Raster_GetHeight(GisAiRaster* raster);
GIS_AI_API int GisAi_Raster_GetBandCount(GisAiRaster* raster);

// ============================================================================
// Vector I/O
// ============================================================================

GIS_AI_API GisAiVector* GisAi_Vector_Load(const char* path);
GIS_AI_API int GisAi_Vector_Save(GisAiVector* vector, const char* path);
GIS_AI_API void GisAi_Vector_Destroy(GisAiVector* vector);

// ============================================================================
// Point Cloud I/O
// ============================================================================

GIS_AI_API GisAiPointCloud* GisAi_PointCloud_Load(const char* path);
GIS_AI_API int GisAi_PointCloud_Save(GisAiPointCloud* pc, const char* path);
GIS_AI_API void GisAi_PointCloud_Destroy(GisAiPointCloud* pc);

// ============================================================================
// Vector GIS Operations
// ============================================================================

GIS_AI_API GisAiVector* GisAi_Vector_Buffer(GisAiVector* vector, double distance);
GIS_AI_API GisAiVector* GisAi_Vector_Intersect(GisAiVector* a, GisAiVector* b);
GIS_AI_API GisAiVector* GisAi_Vector_Clip(GisAiVector* target, GisAiVector* clipper);
GIS_AI_API GisAiVector* GisAi_Vector_Simplify(GisAiVector* vector, double tolerance);

// ============================================================================
// Raster GIS Operations
// ============================================================================

GIS_AI_API GisAiRaster* GisAi_Raster_Resample(GisAiRaster* raster, int new_width, int new_height, const char* method);
GIS_AI_API GisAiRaster* GisAi_Raster_Normalize(GisAiRaster* raster);
GIS_AI_API GisAiRaster* GisAi_Raster_Threshold(GisAiRaster* raster, double threshold);

// ============================================================================
// AI Model Management
// ============================================================================

GIS_AI_API GisAiModel* GisAi_Model_Load(const char* model_path);
GIS_AI_API void GisAi_Model_Unload(GisAiModel* model);
GIS_AI_API GisAiRaster* GisAi_AI_Infer(GisAiModel* model, GisAiRaster* input);

// ============================================================================
// AI Raster Segmentation
// ============================================================================

GIS_AI_API GisAiRasterSeg* GisAi_RasterSeg_Create(const char* model_path);
GIS_AI_API int GisAi_RasterSeg_Run(GisAiRasterSeg* seg, const char* input_tif,
                                    const char* output_tif, const char* output_shp);
GIS_AI_API void GisAi_RasterSeg_Destroy(GisAiRasterSeg* seg);

// ============================================================================
// Coordinate Transformation
// ============================================================================

GIS_AI_API int GisAi_TransformCoordinates(double* x, double* y, const char* from_crs, const char* to_crs);

#ifdef __cplusplus
}
#endif

#endif // GIS_AI_H
