#ifndef GIS_AI_PREPROCESS_H
#define GIS_AI_PREPROCESS_H

#include <vector>
#include "io/raster_io.h"
#include "core/export.h"

namespace gis_ai {

enum class GIS_AI_API NormalizeMode {
    None,
    ImageNet,
    MinMax01,
    ZScore,
    Custom
};

struct GIS_AI_API PreprocessConfig {
    int target_width = 512;
    int target_height = 512;
    int target_channels = 3;
    NormalizeMode normalize_mode = NormalizeMode::ImageNet;
    bool input_is_uint8 = true;
    float mean_r = 0.485f;
    float mean_g = 0.456f;
    float mean_b = 0.406f;
    float std_r = 0.229f;
    float std_g = 0.224f;
    float std_b = 0.225f;
};

class GIS_AI_API Preprocess {
public:
    Preprocess() = default;

    std::vector<float> RasterToTensor(const RasterData& raster, const PreprocessConfig& config = PreprocessConfig());

    std::vector<float> RasterBandToTensor(const RasterData& raster, int band_index, int target_size);

    static std::vector<int64_t> GetInputShape(const PreprocessConfig& config);
};

} // namespace gis_ai

#endif // GIS_AI_PREPROCESS_H
