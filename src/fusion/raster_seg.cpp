#include "fusion/raster_seg.h"
#include "ai/mask_to_polygon.h"
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "core/logger.h"
#include "core/exception.h"

#include <filesystem>

namespace gis_ai {

namespace {

void ValidateSegmentationOutput(const InferenceResult& result, const char* context) {
    if (result.outputs.empty() || result.shapes.empty()) {
        throw GisAiModelException("Model returned no outputs", context);
    }

    const auto& output = result.outputs[0];
    const auto& shape = result.shapes[0];
    if (shape.size() < 4) {
        throw GisAiModelException("Model output shape must have at least 4 dimensions", context);
    }

    const auto expected = static_cast<size_t>(shape[1] * shape[2] * shape[3]);
    if (expected == 0 || output.size() < expected) {
        throw GisAiModelException("Model output tensor size does not match its shape", context);
    }
}

}

RasterSeg::RasterSeg(const std::string& model_path) {
    if (!std::filesystem::exists(model_path)) {
        throw GisAiModelException("Model file not found: " + model_path, "RasterSeg::RasterSeg");
    }

    model_name_ = std::filesystem::path(model_path).stem().string();
    model_manager_.LoadModel(model_path, model_name_);
    engine_ = std::make_unique<InferenceEngine>(model_manager_);
}

RasterSeg::~RasterSeg() = default;

std::unique_ptr<RasterData> RasterSeg::Segment(const RasterData& input) {
    Preprocess preprocess;
    Postprocess postprocess;

    PreprocessConfig pp_config;
    pp_config.target_width = config_.input_size;
    pp_config.target_height = config_.input_size;
    pp_config.mean_r = config_.mean_r;
    pp_config.mean_g = config_.mean_g;
    pp_config.mean_b = config_.mean_b;
    pp_config.std_r = config_.std_r;
    pp_config.std_g = config_.std_g;
    pp_config.std_b = config_.std_b;

    auto tensor = preprocess.RasterToTensor(input, pp_config);
    auto shape = Preprocess::GetInputShape(pp_config);

    auto result = engine_->Run(model_name_, tensor, shape);
    last_inference_time_ms_ = result.inference_time_ms;
    ValidateSegmentationOutput(result, "RasterSeg::Segment");

    auto& output = result.outputs[0];
    auto& out_shape = result.shapes[0];

    int64_t num_classes = out_shape[1];
    int64_t out_h = out_shape[2];
    int64_t out_w = out_shape[3];

    auto mask = postprocess.SigmoidArgmax(output, out_h, out_w, num_classes);

    auto raster = std::make_unique<RasterData>();
    *raster = postprocess.MaskToRaster(mask, static_cast<int>(out_w), static_cast<int>(out_h),
        input.geotransform, input.projection);

    LOG_INFO("Segmentation completed: inference=" + std::to_string(last_inference_time_ms_) + "ms");
    return raster;
}

std::unique_ptr<VectorData> RasterSeg::SegmentToPolygon(const RasterData& input) {
    auto mask_raster = Segment(input);

    MaskToPolygon m2p;
    return m2p.ExecuteFromRaster(*mask_raster, config_.target_class);
}

int RasterSeg::SegmentToFile(const std::string& input_path, const std::string& output_tif,
                               const std::string& output_shp) {
    RasterIO rio;
    auto raster_data = rio.Load(input_path);

    auto result_raster = Segment(*raster_data);
    rio.Save(*result_raster, output_tif);

    if (!output_shp.empty()) {
        auto vector_data = SegmentToPolygon(*raster_data);
        VectorIO vio;
        vio.Save(*vector_data, output_shp);
    }

    LOG_INFO("SegmentToFile completed: " + input_path + " -> " + output_tif);
    return 0;
}

} // namespace gis_ai
