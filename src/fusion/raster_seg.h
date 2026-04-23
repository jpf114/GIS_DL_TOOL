#ifndef GIS_AI_RASTER_SEG_H
#define GIS_AI_RASTER_SEG_H

#include <memory>
#include <string>
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "ai/model_manager.h"
#include "ai/inference_engine.h"
#include "ai/preprocess.h"
#include "ai/postprocess.h"
#include "core/export.h"

namespace gis_ai {

struct GIS_AI_API RasterSegConfig {
    int input_size = 512;
    float mean_r = 0.485f;
    float mean_g = 0.456f;
    float mean_b = 0.406f;
    float std_r = 0.229f;
    float std_g = 0.224f;
    float std_b = 0.225f;
    float mask_threshold = 0.5f;
    uint8_t target_class = 1;
};

class GIS_AI_API RasterSeg {
public:
    explicit RasterSeg(const std::string& model_path);
    ~RasterSeg();

    RasterSeg(const RasterSeg&) = delete;
    RasterSeg& operator=(const RasterSeg&) = delete;

    std::unique_ptr<RasterData> Segment(const RasterData& input);

    std::unique_ptr<VectorData> SegmentToPolygon(const RasterData& input);

    int SegmentToFile(const std::string& input_path, const std::string& output_tif,
                       const std::string& output_shp = "");

    double GetLastInferenceTimeMs() const { return last_inference_time_ms_; }

private:
    ModelManager model_manager_;
    std::unique_ptr<InferenceEngine> engine_;
    std::string model_name_;
    RasterSegConfig config_;
    double last_inference_time_ms_ = 0.0;
};

} // namespace gis_ai

#endif // GIS_AI_RASTER_SEG_H
