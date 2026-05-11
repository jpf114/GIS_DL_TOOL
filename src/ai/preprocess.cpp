#include "ai/preprocess.h"
#include "core/logger.h"
#include "core/exception.h"

#include <cmath>
#include <algorithm>

namespace gis_ai {

std::vector<float> Preprocess::RasterToTensor(const RasterData& raster, const PreprocessConfig& config) {
    if (raster.bands.empty()) {
        throw GisAiAlgorithmException("Input raster has no bands", "Preprocess::RasterToTensor");
    }

    int target_w = config.target_width;
    int target_h = config.target_height;
    int num_channels = config.target_channels;
    int source_channels = std::min(raster.band_count, num_channels);

    std::vector<float> tensor(static_cast<size_t>(num_channels) * target_w * target_h, 0.0f);

    float x_scale = static_cast<float>(raster.width) / target_w;
    float y_scale = static_cast<float>(raster.height) / target_h;

    float means[3] = {config.mean_r, config.mean_g, config.mean_b};
    float stds[3] = {config.std_r, config.std_g, config.std_b};

    for (int c = 0; c < source_channels; ++c) {
        const auto& band = raster.bands[c];

        float band_min = 0.0f;
        float band_max = 1.0f;
        if (config.normalize_mode == NormalizeMode::MinMax01 || config.normalize_mode == NormalizeMode::ZScore) {
            band_min = std::numeric_limits<float>::max();
            band_max = std::numeric_limits<float>::lowest();
            float sum = 0.0f;
            for (float v : band) {
                if (!std::isnan(v)) {
                    band_min = std::min(band_min, v);
                    band_max = std::max(band_max, v);
                    sum += v;
                }
            }
            if (config.normalize_mode == NormalizeMode::ZScore) {
                float mean = sum / static_cast<float>(band.size());
                float sq_sum = 0.0f;
                for (float v : band) {
                    if (!std::isnan(v)) sq_sum += (v - mean) * (v - mean);
                }
                float stddev = std::sqrt(sq_sum / static_cast<float>(band.size()));
                means[c] = mean;
                stds[c] = stddev > 1e-6f ? stddev : 1.0f;
            }
        }

        for (int y = 0; y < target_h; ++y) {
            for (int x = 0; x < target_w; ++x) {
                int src_x = std::min(raster.width - 1, static_cast<int>(x * x_scale));
                int src_y = std::min(raster.height - 1, static_cast<int>(y * y_scale));

                float val = band[src_y * raster.width + src_x];

                if (std::isnan(val)) val = 0.0f;

                switch (config.normalize_mode) {
                    case NormalizeMode::None:
                        break;
                    case NormalizeMode::ImageNet:
                        if (config.input_is_uint8) {
                            val = val / 255.0f;
                        }
                        if (c < 3) {
                            val = (val - means[c]) / stds[c];
                        }
                        break;
                    case NormalizeMode::MinMax01: {
                        float range = band_max - band_min;
                        if (range > 1e-6f) {
                            val = (val - band_min) / range;
                        } else {
                            val = 0.0f;
                        }
                        break;
                    }
                    case NormalizeMode::ZScore:
                        val = (val - means[c]) / stds[c];
                        break;
                    case NormalizeMode::Custom:
                        if (config.input_is_uint8) {
                            val = val / 255.0f;
                        }
                        if (c < 3) {
                            val = (val - means[c]) / stds[c];
                        }
                        break;
                }

                size_t idx = static_cast<size_t>(c) * target_w * target_h + y * target_w + x;
                tensor[idx] = val;
            }
        }
    }

    LOG_INFO("RasterToTensor: " + std::to_string(raster.width) + "x" +
        std::to_string(raster.height) + "x" + std::to_string(source_channels) +
        " -> " + std::to_string(target_w) + "x" + std::to_string(target_h) +
        "x" + std::to_string(num_channels));
    return tensor;
}

std::vector<float> Preprocess::RasterBandToTensor(const RasterData& raster, int band_index, int target_size) {
    if (band_index < 0 || band_index >= raster.band_count) {
        throw GisAiAlgorithmException("Band index out of range", "Preprocess::RasterBandToTensor");
    }

    const auto& band = raster.bands[band_index];
    std::vector<float> tensor(static_cast<size_t>(target_size) * target_size, 0.0f);

    float x_scale = static_cast<float>(raster.width) / target_size;
    float y_scale = static_cast<float>(raster.height) / target_size;

    for (int y = 0; y < target_size; ++y) {
        for (int x = 0; x < target_size; ++x) {
            int src_x = std::min(raster.width - 1, static_cast<int>(x * x_scale));
            int src_y = std::min(raster.height - 1, static_cast<int>(y * y_scale));

            float val = band[src_y * raster.width + src_x];
            if (std::isnan(val)) val = 0.0f;

            tensor[y * target_size + x] = val;
        }
    }

    return tensor;
}

std::vector<int64_t> Preprocess::GetInputShape(const PreprocessConfig& config) {
    return {1, static_cast<int64_t>(config.target_channels), config.target_height, config.target_width};
}

} // namespace gis_ai
