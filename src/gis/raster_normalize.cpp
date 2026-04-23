#include "gis/raster_normalize.h"
#include "core/logger.h"
#include "core/exception.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gis_ai {

std::unique_ptr<RasterData> RasterNormalize::Execute(const RasterData& input) {
    if (input.bands.empty()) {
        throw GisAiAlgorithmException("Input raster has no bands", "RasterNormalize::Execute");
    }

    auto result = std::make_unique<RasterData>();
    result->width = input.width;
    result->height = input.height;
    result->band_count = input.band_count;
    result->projection = input.projection;
    memcpy(result->geotransform, input.geotransform, sizeof(double) * 6);

    result->bands.resize(input.band_count);

    for (int b = 0; b < input.band_count; ++b) {
        const auto& src_band = input.bands[b];
        size_t total = static_cast<size_t>(input.width) * input.height;

        float min_val = std::numeric_limits<float>::max();
        float max_val = std::numeric_limits<float>::lowest();

        for (size_t i = 0; i < total; ++i) {
            if (!std::isnan(src_band[i])) {
                min_val = std::min(min_val, src_band[i]);
                max_val = std::max(max_val, src_band[i]);
            }
        }

        result->bands[b].resize(total);

        float range = max_val - min_val;
        if (range < 1e-10f) {
            for (size_t i = 0; i < total; ++i) {
                result->bands[b][i] = 0.0f;
            }
            LOG_WARN("Band " + std::to_string(b) + " has zero range, normalized to 0");
            continue;
        }

        for (size_t i = 0; i < total; ++i) {
            if (std::isnan(src_band[i])) {
                result->bands[b][i] = std::numeric_limits<float>::quiet_NaN();
            } else {
                result->bands[b][i] = (src_band[i] - min_val) / range;
            }
        }
    }

    LOG_INFO("Normalize completed: " + std::to_string(input.band_count) + " bands normalized to [0,1]");
    return result;
}

std::unique_ptr<RasterData> RasterNormalize::ExecuteMinMax(const RasterData& input, float min_val, float max_val) {
    if (input.bands.empty()) {
        throw GisAiAlgorithmException("Input raster has no bands", "RasterNormalize::ExecuteMinMax");
    }
    if (max_val <= min_val) {
        throw GisAiAlgorithmException("max_val must be greater than min_val", "RasterNormalize::ExecuteMinMax");
    }

    auto result = std::make_unique<RasterData>();
    result->width = input.width;
    result->height = input.height;
    result->band_count = input.band_count;
    result->projection = input.projection;
    memcpy(result->geotransform, input.geotransform, sizeof(double) * 6);

    result->bands.resize(input.band_count);

    for (int b = 0; b < input.band_count; ++b) {
        const auto& src_band = input.bands[b];
        size_t total = static_cast<size_t>(input.width) * input.height;
        result->bands[b].resize(total);

        float range = max_val - min_val;
        for (size_t i = 0; i < total; ++i) {
            if (std::isnan(src_band[i])) {
                result->bands[b][i] = std::numeric_limits<float>::quiet_NaN();
            } else {
                float clamped = std::max(min_val, std::min(max_val, src_band[i]));
                result->bands[b][i] = (clamped - min_val) / range;
            }
        }
    }

    LOG_INFO("MinMax normalize completed: range [" + std::to_string(min_val) +
        ", " + std::to_string(max_val) + "] -> [0,1]");
    return result;
}

} // namespace gis_ai
