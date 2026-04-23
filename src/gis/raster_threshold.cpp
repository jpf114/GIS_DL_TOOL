#include "gis/raster_threshold.h"
#include "core/logger.h"
#include "core/exception.h"

#include <cmath>

namespace gis_ai {

std::unique_ptr<RasterData> RasterThreshold::Execute(const RasterData& input, double threshold, int band_index) {
    if (band_index < 0 || band_index >= input.band_count) {
        throw GisAiAlgorithmException("Band index out of range: " + std::to_string(band_index),
            "RasterThreshold::Execute");
    }

    auto result = std::make_unique<RasterData>();
    result->width = input.width;
    result->height = input.height;
    result->band_count = 1;
    result->projection = input.projection;
    memcpy(result->geotransform, input.geotransform, sizeof(double) * 6);

    size_t total = static_cast<size_t>(input.width) * input.height;
    result->bands.resize(1);
    result->bands[0].resize(total);

    const auto& src = input.bands[band_index];
    auto& dst = result->bands[0];

    for (size_t i = 0; i < total; ++i) {
        if (std::isnan(src[i])) {
            dst[i] = 0.0f;
        } else {
            dst[i] = (src[i] >= static_cast<float>(threshold)) ? 1.0f : 0.0f;
        }
    }

    LOG_INFO("Threshold completed: threshold=" + std::to_string(threshold) +
        ", band=" + std::to_string(band_index));
    return result;
}

std::unique_ptr<RasterData> RasterThreshold::ExecuteRange(const RasterData& input, double min_val, double max_val, int band_index) {
    if (band_index < 0 || band_index >= input.band_count) {
        throw GisAiAlgorithmException("Band index out of range: " + std::to_string(band_index),
            "RasterThreshold::ExecuteRange");
    }
    if (min_val > max_val) {
        throw GisAiAlgorithmException("min_val must be <= max_val", "RasterThreshold::ExecuteRange");
    }

    auto result = std::make_unique<RasterData>();
    result->width = input.width;
    result->height = input.height;
    result->band_count = 1;
    result->projection = input.projection;
    memcpy(result->geotransform, input.geotransform, sizeof(double) * 6);

    size_t total = static_cast<size_t>(input.width) * input.height;
    result->bands.resize(1);
    result->bands[0].resize(total);

    const auto& src = input.bands[band_index];
    auto& dst = result->bands[0];

    float fmin = static_cast<float>(min_val);
    float fmax = static_cast<float>(max_val);

    for (size_t i = 0; i < total; ++i) {
        if (std::isnan(src[i])) {
            dst[i] = 0.0f;
        } else {
            dst[i] = (src[i] >= fmin && src[i] <= fmax) ? 1.0f : 0.0f;
        }
    }

    LOG_INFO("Range threshold completed: [" + std::to_string(min_val) +
        ", " + std::to_string(max_val) + "], band=" + std::to_string(band_index));
    return result;
}

} // namespace gis_ai
