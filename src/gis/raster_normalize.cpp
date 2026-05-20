#include "gis/raster_normalize.h"
#include "core/logger.h"
#include "core/exception.h"

#include <algorithm>
#include <cmath>
#include <limits>

#ifdef _OPENMP
#include <omp.h>
#endif

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
    result->band_infos = input.band_infos;
    memcpy(result->geotransform, input.geotransform, sizeof(double) * 6);

    result->bands.resize(input.band_count);

    for (int b = 0; b < input.band_count; ++b) {
        const auto& src_band = input.bands[b];
        size_t total = static_cast<size_t>(input.width) * input.height;

        float min_val = std::numeric_limits<float>::max();
        float max_val = std::numeric_limits<float>::lowest();
        bool has_valid = false;

        #pragma omp parallel
        {
            float local_min = std::numeric_limits<float>::max();
            float local_max = std::numeric_limits<float>::lowest();
            bool local_has_valid = false;

            #pragma omp for schedule(static) nowait
            for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(total); ++i) {
                auto si = static_cast<size_t>(i);
                if (!std::isnan(src_band[si])) {
                    local_min = std::min(local_min, src_band[si]);
                    local_max = std::max(local_max, src_band[si]);
                    local_has_valid = true;
                }
            }

            #pragma omp critical(reduce_minmax)
            {
                min_val = std::min(min_val, local_min);
                max_val = std::max(max_val, local_max);
                has_valid = has_valid || local_has_valid;
            }
        }

        result->bands[b].resize(total);

        if (!has_valid) {
            for (size_t i = 0; i < total; ++i) {
                result->bands[b][i] = std::numeric_limits<float>::quiet_NaN();
            }
            LOG_WARN("Band " + std::to_string(b) + " is all NaN, preserved as NaN");
            continue;
        }

        float range = max_val - min_val;
        if (range < 1e-10f) {
            #pragma omp parallel for schedule(static)
            for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(total); ++i) {
                auto si = static_cast<size_t>(i);
                if (std::isnan(src_band[si])) {
                    result->bands[b][si] = std::numeric_limits<float>::quiet_NaN();
                } else {
                    result->bands[b][si] = 0.0f;
                }
            }
            LOG_WARN("Band " + std::to_string(b) + " has zero range, normalized to 0");
            continue;
        }

        #pragma omp parallel for schedule(static)
        for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(total); ++i) {
            auto si = static_cast<size_t>(i);
            if (std::isnan(src_band[si])) {
                result->bands[b][si] = std::numeric_limits<float>::quiet_NaN();
            } else {
                result->bands[b][si] = (src_band[si] - min_val) / range;
            }
        }
    }

    for (int b = 0; b < static_cast<int>(result->band_infos.size()); ++b) {
        result->band_infos[b].nodata_value = std::nullopt;
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
    result->band_infos = input.band_infos;
    memcpy(result->geotransform, input.geotransform, sizeof(double) * 6);

    result->bands.resize(input.band_count);

    for (int b = 0; b < input.band_count; ++b) {
        const auto& src_band = input.bands[b];
        size_t total = static_cast<size_t>(input.width) * input.height;
        result->bands[b].resize(total);

        float range = max_val - min_val;
        #pragma omp parallel for schedule(static)
        for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(total); ++i) {
            auto si = static_cast<size_t>(i);
            if (std::isnan(src_band[si])) {
                result->bands[b][si] = std::numeric_limits<float>::quiet_NaN();
            } else {
                float clamped = std::max(min_val, std::min(max_val, src_band[si]));
                result->bands[b][si] = (clamped - min_val) / range;
            }
        }
    }

    for (int b = 0; b < static_cast<int>(result->band_infos.size()); ++b) {
        result->band_infos[b].nodata_value = std::nullopt;
    }

    LOG_INFO("MinMax normalize completed: range [" + std::to_string(min_val) +
        ", " + std::to_string(max_val) + "] -> [0,1]");
    return result;
}

} // namespace gis_ai
