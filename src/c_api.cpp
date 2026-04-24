#include "gis_ai/gis_ai.h"

#include "core/logger.h"
#include "core/exception.h"
#include "core/config.h"
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "io/pointcloud_io.h"
#include "io/io_factory.h"
#include "gis/vector_buffer.h"
#include "gis/vector_intersect.h"
#include "gis/vector_clip.h"
#include "gis/vector_simplify.h"
#include "gis/raster_resample.h"
#include "gis/raster_normalize.h"
#include "gis/raster_threshold.h"
#include "gis/coord_transform.h"
#include "ai/model_manager.h"
#include "ai/inference_engine.h"
#include "ai/preprocess.h"
#include "ai/postprocess.h"
#include "ai/mask_to_polygon.h"

#include <string>
#include <cstring>
#include <memory>
#include <filesystem>

namespace {

thread_local std::string g_last_error;
thread_local int g_last_error_code = 0;
thread_local std::string g_projection_cache;

void SetError(int code, const std::string& msg) {
    g_last_error = msg;
    g_last_error_code = code;
}

void ClearError() {
    g_last_error.clear();
    g_last_error_code = 0;
}

static gis_ai::ModelManager g_model_manager;

constexpr int VERSION_MAJOR = 0;
constexpr int VERSION_MINOR = 1;
constexpr int VERSION_PATCH = 0;
constexpr const char* VERSION_STRING = "0.1.0";

}

struct GisAiRaster {
    gis_ai::RasterData data;
};

struct GisAiVector {
    gis_ai::VectorData data;
};

struct GisAiPointCloud {
    gis_ai::PointCloudData data;
};

struct GisAiModel {
    std::string name;
    gis_ai::ModelManager* manager;
};

struct GisAiRasterSeg {
    std::unique_ptr<gis_ai::ModelManager> manager;
    std::unique_ptr<gis_ai::InferenceEngine> engine;
    std::string model_name;
};

extern "C" {

GIS_AI_API int GisAi_Init(const char* config_path) {
    try {
        gis_ai::Logger::Instance().Initialize("gis_ai.log");
        if (config_path) {
            gis_ai::Config::Instance().LoadFromFile(config_path);
        }
        ClearError();
        return 0;
    } catch (const std::exception& e) {
        SetError(-1, e.what());
        return -1;
    }
}

GIS_AI_API void GisAi_Shutdown() {
    g_last_error.clear();
    g_last_error_code = 0;
}

GIS_AI_API const char* GisAi_GetLastError() {
    return g_last_error.c_str();
}

GIS_AI_API int GisAi_GetLastErrorCode() {
    return g_last_error_code;
}

GIS_AI_API int GisAi_GetVersionMajor() { return VERSION_MAJOR; }
GIS_AI_API int GisAi_GetVersionMinor() { return VERSION_MINOR; }
GIS_AI_API int GisAi_GetVersionPatch() { return VERSION_PATCH; }
GIS_AI_API const char* GisAi_GetVersionString() { return VERSION_STRING; }

GIS_AI_API GisAiRaster* GisAi_Raster_Load(const char* path) {
    if (!path) { SetError(-2, "Null path"); return nullptr; }
    try {
        gis_ai::RasterIO io;
        auto data = io.Load(path);
        auto* result = new GisAiRaster();
        result->data = std::move(*data);
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API int GisAi_Raster_Save(GisAiRaster* raster, const char* path) {
    if (!raster || !path) { SetError(-2, "Null argument"); return -2; }
    try {
        gis_ai::RasterIO io;
        io.Save(raster->data, path);
        ClearError();
        return 0;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return -3;
    }
}

GIS_AI_API void GisAi_Raster_Destroy(GisAiRaster* raster) {
    delete raster;
}

GIS_AI_API int GisAi_Raster_GetWidth(GisAiRaster* raster) {
    return raster ? raster->data.width : 0;
}

GIS_AI_API int GisAi_Raster_GetHeight(GisAiRaster* raster) {
    return raster ? raster->data.height : 0;
}

GIS_AI_API int GisAi_Raster_GetBandCount(GisAiRaster* raster) {
    return raster ? raster->data.band_count : 0;
}

GIS_AI_API int GisAi_Raster_GetGeoTransform(GisAiRaster* raster, double* out_transform) {
    if (!raster || !out_transform) { SetError(-2, "Null argument"); return -2; }
    std::memcpy(out_transform, raster->data.geotransform, 6 * sizeof(double));
    ClearError();
    return 0;
}

GIS_AI_API const char* GisAi_Raster_GetProjection(GisAiRaster* raster) {
    if (!raster) return nullptr;
    g_projection_cache = raster->data.projection;
    return g_projection_cache.c_str();
}

GIS_AI_API int GisAi_Raster_GetBandData(GisAiRaster* raster, int band_index, float* out_buffer, int buffer_size) {
    if (!raster || !out_buffer) { SetError(-2, "Null argument"); return -2; }
    if (band_index < 0 || band_index >= raster->data.band_count) { SetError(-4, "Band index out of range"); return -4; }
    auto& band = raster->data.bands[band_index];
    int copy_size = std::min(buffer_size, static_cast<int>(band.size()));
    std::memcpy(out_buffer, band.data(), copy_size * sizeof(float));
    ClearError();
    return copy_size;
}

GIS_AI_API GisAiVector* GisAi_Vector_Load(const char* path) {
    if (!path) { SetError(-2, "Null path"); return nullptr; }
    try {
        gis_ai::VectorIO io;
        auto data = io.Load(path);
        auto* result = new GisAiVector();
        result->data = std::move(*data);
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API int GisAi_Vector_Save(GisAiVector* vector, const char* path) {
    if (!vector || !path) { SetError(-2, "Null argument"); return -2; }
    try {
        gis_ai::VectorIO io;
        io.Save(vector->data, path);
        ClearError();
        return 0;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return -3;
    }
}

GIS_AI_API void GisAi_Vector_Destroy(GisAiVector* vector) {
    delete vector;
}

GIS_AI_API int GisAi_Vector_GetFeatureCount(GisAiVector* vector) {
    return vector ? static_cast<int>(vector->data.features.size()) : 0;
}

GIS_AI_API const char* GisAi_Vector_GetProjection(GisAiVector* vector) {
    if (!vector) return nullptr;
    g_projection_cache = vector->data.projection;
    return g_projection_cache.c_str();
}

GIS_AI_API GisAiPointCloud* GisAi_PointCloud_Load(const char* path) {
    if (!path) { SetError(-2, "Null path"); return nullptr; }
    try {
        gis_ai::PointCloudIO io;
        auto data = io.Load(path);
        auto* result = new GisAiPointCloud();
        result->data = std::move(*data);
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API int GisAi_PointCloud_Save(GisAiPointCloud* pc, const char* path) {
    if (!pc || !path) { SetError(-2, "Null argument"); return -2; }
    try {
        gis_ai::PointCloudIO io;
        io.Save(pc->data, path);
        ClearError();
        return 0;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return -3;
    }
}

GIS_AI_API void GisAi_PointCloud_Destroy(GisAiPointCloud* pc) {
    delete pc;
}

GIS_AI_API int GisAi_PointCloud_GetPointCount(GisAiPointCloud* pc) {
    return pc ? static_cast<int>(pc->data.points.size()) : 0;
}

GIS_AI_API GisAiVector* GisAi_Vector_Buffer(GisAiVector* vector, double distance) {
    if (!vector) { SetError(-2, "Null vector"); return nullptr; }
    try {
        gis_ai::VectorBuffer buffer;
        auto result_data = buffer.Execute(vector->data, distance);
        auto* result = new GisAiVector();
        result->data = std::move(*result_data);
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API GisAiVector* GisAi_Vector_Intersect(GisAiVector* a, GisAiVector* b) {
    if (!a || !b) { SetError(-2, "Null argument"); return nullptr; }
    try {
        gis_ai::VectorIntersect intersect;
        auto result_data = intersect.Execute(a->data, b->data);
        auto* result = new GisAiVector();
        result->data = std::move(*result_data);
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API GisAiVector* GisAi_Vector_Clip(GisAiVector* target, GisAiVector* clipper) {
    if (!target || !clipper) { SetError(-2, "Null argument"); return nullptr; }
    try {
        gis_ai::VectorClip clip;
        auto result_data = clip.Execute(target->data, clipper->data);
        auto* result = new GisAiVector();
        result->data = std::move(*result_data);
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API GisAiVector* GisAi_Vector_Simplify(GisAiVector* vector, double tolerance) {
    if (!vector) { SetError(-2, "Null vector"); return nullptr; }
    try {
        gis_ai::VectorSimplify simplify;
        auto result_data = simplify.Execute(vector->data, tolerance);
        auto* result = new GisAiVector();
        result->data = std::move(*result_data);
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API GisAiRaster* GisAi_Raster_Resample(GisAiRaster* raster, int new_width, int new_height, const char* method) {
    if (!raster) { SetError(-2, "Null raster"); return nullptr; }
    try {
        gis_ai::ResampleMethod rm = gis_ai::ResampleMethod::Nearest;
        if (method && std::strcmp(method, "bilinear") == 0) {
            rm = gis_ai::ResampleMethod::Bilinear;
        }
        gis_ai::RasterResample resample;
        auto result_data = resample.Execute(raster->data, new_width, new_height, rm);
        auto* result = new GisAiRaster();
        result->data = std::move(*result_data);
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API GisAiRaster* GisAi_Raster_Normalize(GisAiRaster* raster) {
    if (!raster) { SetError(-2, "Null raster"); return nullptr; }
    try {
        gis_ai::RasterNormalize normalize;
        auto result_data = normalize.Execute(raster->data);
        auto* result = new GisAiRaster();
        result->data = std::move(*result_data);
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API GisAiRaster* GisAi_Raster_Threshold(GisAiRaster* raster, double threshold) {
    if (!raster) { SetError(-2, "Null raster"); return nullptr; }
    try {
        gis_ai::RasterThreshold thresh;
        auto result_data = thresh.Execute(raster->data, threshold);
        auto* result = new GisAiRaster();
        result->data = std::move(*result_data);
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API GisAiModel* GisAi_Model_Load(const char* model_path) {
    if (!model_path) { SetError(-2, "Null model_path"); return nullptr; }
    try {
        std::string name = std::filesystem::path(model_path).stem().string();
        g_model_manager.LoadModel(model_path, name);
        auto* result = new GisAiModel();
        result->name = name;
        result->manager = &g_model_manager;
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API void GisAi_Model_Unload(GisAiModel* model) {
    if (model) {
        model->manager->UnloadModel(model->name);
        delete model;
    }
}

GIS_AI_API GisAiRaster* GisAi_AI_Infer(GisAiModel* model, GisAiRaster* input) {
    if (!model || !input) { SetError(-2, "Null argument"); return nullptr; }
    try {
        gis_ai::InferenceEngine engine(*model->manager);
        gis_ai::Preprocess preprocess;
        gis_ai::Postprocess postprocess;

        gis_ai::PreprocessConfig config;
        auto tensor = preprocess.RasterToTensor(input->data, config);
        auto shape = gis_ai::Preprocess::GetInputShape(config);

        auto infer_result = engine.Run(model->name, tensor, shape);

        auto& output = infer_result.outputs[0];
        auto& out_shape = infer_result.shapes[0];

        int64_t num_classes = out_shape[1];
        int64_t out_h = out_shape[2];
        int64_t out_w = out_shape[3];

        auto mask = postprocess.SigmoidArgmax(output, out_h, out_w, num_classes);

        auto* result = new GisAiRaster();
        result->data = postprocess.MaskToRaster(mask, static_cast<int>(out_w), static_cast<int>(out_h),
            input->data.geotransform, input->data.projection);
        ClearError();
        return result;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API GisAiRasterSeg* GisAi_RasterSeg_Create(const char* model_path) {
    if (!model_path) { SetError(-2, "Null model_path"); return nullptr; }
    try {
        auto* seg = new GisAiRasterSeg();
        seg->manager = std::make_unique<gis_ai::ModelManager>();
        seg->engine = std::make_unique<gis_ai::InferenceEngine>(*seg->manager);

        std::string name = std::filesystem::path(model_path).stem().string();
        seg->manager->LoadModel(model_path, name);
        seg->model_name = name;
        ClearError();
        return seg;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return nullptr;
    }
}

GIS_AI_API int GisAi_RasterSeg_Run(GisAiRasterSeg* seg, const char* input_tif,
                                     const char* output_tif, const char* output_shp) {
    if (!seg || !input_tif || !output_tif) { SetError(-2, "Null argument"); return -2; }
    try {
        gis_ai::RasterIO rio;
        auto raster_data = rio.Load(input_tif);

        gis_ai::Preprocess preprocess;
        gis_ai::Postprocess postprocess;

        gis_ai::PreprocessConfig config;
        auto tensor = preprocess.RasterToTensor(*raster_data, config);
        auto shape = gis_ai::Preprocess::GetInputShape(config);

        auto infer_result = seg->engine->Run(seg->model_name, tensor, shape);

        auto& output = infer_result.outputs[0];
        auto& out_shape = infer_result.shapes[0];

        int64_t num_classes = out_shape[1];
        int64_t out_h = out_shape[2];
        int64_t out_w = out_shape[3];

        auto mask = postprocess.SigmoidArgmax(output, out_h, out_w, num_classes);
        auto result_raster = postprocess.MaskToRaster(mask, static_cast<int>(out_w), static_cast<int>(out_h),
            raster_data->geotransform, raster_data->projection);

        rio.Save(result_raster, output_tif);

        if (output_shp) {
            gis_ai::MaskToPolygon m2p;
            auto vector_data = m2p.Execute(mask, static_cast<int>(out_w), static_cast<int>(out_h),
                raster_data->geotransform, 1);

            gis_ai::VectorIO vio;
            vio.Save(*vector_data, output_shp);
        }

        ClearError();
        return 0;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return -3;
    }
}

GIS_AI_API void GisAi_RasterSeg_Destroy(GisAiRasterSeg* seg) {
    delete seg;
}

GIS_AI_API int GisAi_TransformCoordinates(double* x, double* y, const char* from_crs, const char* to_crs) {
    if (!x || !y || !from_crs || !to_crs) { SetError(-2, "Null argument"); return -2; }
    try {
        gis_ai::CoordTransform transform;
        auto result = transform.Transform(*x, *y, 0.0, from_crs, to_crs);
        *x = result.x;
        *y = result.y;
        ClearError();
        return 0;
    } catch (const std::exception& e) {
        SetError(-3, e.what());
        return -3;
    }
}

} // extern "C"
