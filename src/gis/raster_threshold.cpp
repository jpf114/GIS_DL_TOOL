#include "gis/raster_threshold.h"
#include "core/logger.h"
#include "core/exception.h"

#include <cmath>

#ifdef _OPENMP
#include <omp.h>
#endif

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
    if (band_index < static_cast<int>(input.band_infos.size())) {
        result->band_infos = {input.band_infos[band_index]};
        result->band_infos[0].nodata_value = std::nullopt;
    }
    memcpy(result->geotransform, input.geotransform, sizeof(double) * 6);

    size_t total = static_cast<size_t>(input.width) * input.height;
    result->bands.resize(1);
    result->bands[0].resize(total);

    const auto& src = input.bands[band_index];
    auto& dst = result->bands[0];

    bool has_nodata = band_index < static_cast<int>(input.band_infos.size()) &&
                      input.band_infos[band_index].nodata_value.has_value();
    float nodata_val = has_nodata ? input.band_infos[band_index].nodata_value.value() : 0.0f;

    #pragma omp parallel for schedule(static)
    for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(total); ++i) {
        auto si = static_cast<size_t>(i);
        if (std::isnan(src[si])) {
            dst[si] = 0.0f;
        } else if (has_nodata && !std::isnan(nodata_val) && std::abs(src[si] - nodata_val) < 1e-6f) {
            dst[si] = 0.0f;
        } else {
            dst[si] = (src[si] >= static_cast<float>(threshold)) ? 1.0f : 0.0f;
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
    if (band_index < static_cast<int>(input.band_infos.size())) {
        result->band_infos = {input.band_infos[band_index]};
        result->band_infos[0].nodata_value = std::nullopt;
    }
    memcpy(result->geotransform, input.geotransform, sizeof(double) * 6);

    size_t total = static_cast<size_t>(input.width) * input.height;
    result->bands.resize(1);
    result->bands[0].resize(total);

    const auto& src = input.bands[band_index];
    auto& dst = result->bands[0];

    float fmin = static_cast<float>(min_val);
    float fmax = static_cast<float>(max_val);

    bool has_nodata = band_index < static_cast<int>(input.band_infos.size()) &&
                      input.band_infos[band_index].nodata_value.has_value();
    float nodata_val = has_nodata ? input.band_infos[band_index].nodata_value.value() : 0.0f;

    #pragma omp parallel for schedule(static)
    for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(total); ++i) {
        auto si = static_cast<size_t>(i);
        if (std::isnan(src[si])) {
            dst[si] = 0.0f;
        } else if (has_nodata && !std::isnan(nodata_val) && std::abs(src[si] - nodata_val) < 1e-6f) {
            dst[si] = 0.0f;
        } else {
            dst[si] = (src[si] >= fmin && src[si] <= fmax) ? 1.0f : 0.0f;
        }
    }

    LOG_INFO("Range threshold completed: [" + std::to_string(min_val) +
        ", " + std::to_string(max_val) + "], band=" + std::to_string(band_index));
    return result;
}

} // namespace gis_ai
