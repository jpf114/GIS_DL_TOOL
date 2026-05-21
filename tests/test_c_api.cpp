#include <gtest/gtest.h>

#include "gis_ai/gis_ai.h"
#include "gis_ai/version.h"

TEST(CApiVersion, GetVersionMajor) {
    EXPECT_EQ(GisAi_GetVersionMajor(), GIS_AI_VERSION_MAJOR);
}

TEST(CApiVersion, GetVersionMinor) {
    EXPECT_EQ(GisAi_GetVersionMinor(), GIS_AI_VERSION_MINOR);
}

TEST(CApiVersion, GetVersionPatch) {
    EXPECT_EQ(GisAi_GetVersionPatch(), GIS_AI_VERSION_PATCH);
}

TEST(CApiVersion, GetVersionString) {
    const char* ver = GisAi_GetVersionString();
    ASSERT_NE(ver, nullptr);
    std::string expected = std::to_string(GIS_AI_VERSION_MAJOR) + "." +
                           std::to_string(GIS_AI_VERSION_MINOR) + "." +
                           std::to_string(GIS_AI_VERSION_PATCH);
    EXPECT_STREQ(ver, expected.c_str());
}

TEST(CApiInit, InitWithNullPath) {
    int ret = GisAi_Init(nullptr);
    EXPECT_EQ(ret, 0);
    GisAi_Shutdown();
}

TEST(CApiInit, InitWithNonexistentPath) {
    int ret = GisAi_Init("/nonexistent/path/config.json");
    EXPECT_EQ(ret, 0);
    GisAi_Shutdown();
}

TEST(CApiError, GetLastErrorAfterSuccess) {
    GisAi_Init(nullptr);
    EXPECT_EQ(GisAi_GetLastErrorCode(), 0);
    GisAi_Shutdown();
}

TEST(CApiRaster, LoadNullPath) {
    auto* raster = GisAi_Raster_Load(nullptr);
    EXPECT_EQ(raster, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiRaster, LoadNonexistentFile) {
    auto* raster = GisAi_Raster_Load("/nonexistent/file.tif");
    EXPECT_EQ(raster, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
    EXPECT_NE(GisAi_GetLastError(), nullptr);
}

TEST(CApiRaster, GetPropertiesNullRaster) {
    EXPECT_EQ(GisAi_Raster_GetWidth(nullptr), 0);
    EXPECT_EQ(GisAi_Raster_GetHeight(nullptr), 0);
    EXPECT_EQ(GisAi_Raster_GetBandCount(nullptr), 0);
    EXPECT_EQ(GisAi_Raster_GetProjection(nullptr), nullptr);
}

TEST(CApiRaster, GetGeoTransformNullArgs) {
    double transform[6] = {};
    EXPECT_NE(GisAi_Raster_GetGeoTransform(nullptr, transform), 0);
    EXPECT_NE(GisAi_Raster_GetGeoTransform(reinterpret_cast<GisAiRaster*>(1), nullptr), 0);
}

TEST(CApiRaster, GetBandDataNullArgs) {
    float buffer[10] = {};
    EXPECT_NE(GisAi_Raster_GetBandData(nullptr, 0, buffer, 10), 0);
    EXPECT_NE(GisAi_Raster_GetBandData(reinterpret_cast<GisAiRaster*>(1), 0, nullptr, 10), 0);
}

TEST(CApiRaster, SaveNullArgs) {
    EXPECT_NE(GisAi_Raster_Save(nullptr, "test.tif"), 0);
    EXPECT_NE(GisAi_Raster_Save(reinterpret_cast<GisAiRaster*>(1), nullptr), 0);
}

TEST(CApiRaster, DestroyNull) {
    GisAi_Raster_Destroy(nullptr);
}

TEST(CApiVector, LoadNullPath) {
    auto* vec = GisAi_Vector_Load(nullptr);
    EXPECT_EQ(vec, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiVector, LoadNonexistentFile) {
    auto* vec = GisAi_Vector_Load("/nonexistent/file.shp");
    EXPECT_EQ(vec, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiVector, GetPropertiesNullVector) {
    EXPECT_EQ(GisAi_Vector_GetFeatureCount(nullptr), 0);
    EXPECT_EQ(GisAi_Vector_GetProjection(nullptr), nullptr);
}

TEST(CApiVector, SaveNullArgs) {
    EXPECT_NE(GisAi_Vector_Save(nullptr, "test.shp"), 0);
    EXPECT_NE(GisAi_Vector_Save(reinterpret_cast<GisAiVector*>(1), nullptr), 0);
}

TEST(CApiVector, DestroyNull) {
    GisAi_Vector_Destroy(nullptr);
}

TEST(CApiVectorOps, BufferNullVector) {
    auto* result = GisAi_Vector_Buffer(nullptr, 10.0);
    EXPECT_EQ(result, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiVectorOps, IntersectNullArgs) {
    auto* result = GisAi_Vector_Intersect(nullptr, nullptr);
    EXPECT_EQ(result, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiVectorOps, ClipNullArgs) {
    auto* result = GisAi_Vector_Clip(nullptr, nullptr);
    EXPECT_EQ(result, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiVectorOps, SimplifyNullVector) {
    auto* result = GisAi_Vector_Simplify(nullptr, 1.0);
    EXPECT_EQ(result, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiPointCloud, LoadNullPath) {
    auto* pc = GisAi_PointCloud_Load(nullptr);
    EXPECT_EQ(pc, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiPointCloud, GetPointCountNull) {
    EXPECT_EQ(GisAi_PointCloud_GetPointCount(nullptr), 0);
}

TEST(CApiPointCloud, SaveNullArgs) {
    EXPECT_NE(GisAi_PointCloud_Save(nullptr, "test.las"), 0);
}

TEST(CApiPointCloud, DestroyNull) {
    GisAi_PointCloud_Destroy(nullptr);
}

TEST(CApiRasterOps, ResampleNullRaster) {
    auto* result = GisAi_Raster_Resample(nullptr, 100, 100, "nearest");
    EXPECT_EQ(result, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiRasterOps, NormalizeNullRaster) {
    auto* result = GisAi_Raster_Normalize(nullptr);
    EXPECT_EQ(result, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiRasterOps, ThresholdNullRaster) {
    auto* result = GisAi_Raster_Threshold(nullptr, 0.5);
    EXPECT_EQ(result, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiModel, LoadNullPath) {
    auto* model = GisAi_Model_Load(nullptr);
    EXPECT_EQ(model, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiModel, LoadNonexistentModel) {
    auto* model = GisAi_Model_Load("/nonexistent/model.onnx");
    EXPECT_EQ(model, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiModel, UnloadNull) {
    GisAi_Model_Unload(nullptr);
}

TEST(CApiInfer, InferNullArgs) {
    auto* result = GisAi_AI_Infer(nullptr, nullptr);
    EXPECT_EQ(result, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiRasterSeg, CreateNullPath) {
    auto* seg = GisAi_RasterSeg_Create(nullptr);
    EXPECT_EQ(seg, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiRasterSeg, CreateNonexistentModel) {
    auto* seg = GisAi_RasterSeg_Create("/nonexistent/model.onnx");
    EXPECT_EQ(seg, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiRasterSeg, RunNullArgs) {
    EXPECT_NE(GisAi_RasterSeg_Run(nullptr, "in.tif", "out.tif", nullptr), 0);
    auto* seg = GisAi_RasterSeg_Create("/nonexistent/model.onnx");
    if (seg) {
        EXPECT_NE(GisAi_RasterSeg_Run(seg, nullptr, "out.tif", nullptr), 0);
        GisAi_RasterSeg_Destroy(seg);
    }
}

TEST(CApiRasterSeg, DestroyNull) {
    GisAi_RasterSeg_Destroy(nullptr);
}

TEST(CApiLargeImageSeg, CreateNullPath) {
    auto* seg = GisAi_LargeImageSeg_Create(nullptr);
    EXPECT_EQ(seg, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiLargeImageSeg, CreateNonexistentModel) {
    auto* seg = GisAi_LargeImageSeg_Create("/nonexistent/model.onnx");
    EXPECT_EQ(seg, nullptr);
    EXPECT_NE(GisAi_GetLastErrorCode(), 0);
}

TEST(CApiLargeImageSeg, RunNullArgs) {
    EXPECT_NE(GisAi_LargeImageSeg_Run(nullptr, "in.tif", "out.tif", "out.shp", 512, 384, 2), 0);
}

TEST(CApiLargeImageSeg, DestroyNull) {
    GisAi_LargeImageSeg_Destroy(nullptr);
}

TEST(CApiCoordTransform, TransformNullArgs) {
    double x = 116.0, y = 39.0;
    EXPECT_NE(GisAi_TransformCoordinates(nullptr, &y, "EPSG:4326", "EPSG:3857"), 0);
    EXPECT_NE(GisAi_TransformCoordinates(&x, nullptr, "EPSG:4326", "EPSG:3857"), 0);
    EXPECT_NE(GisAi_TransformCoordinates(&x, &y, nullptr, "EPSG:3857"), 0);
    EXPECT_NE(GisAi_TransformCoordinates(&x, &y, "EPSG:4326", nullptr), 0);
}
