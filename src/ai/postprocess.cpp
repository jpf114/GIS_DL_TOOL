#include "ai/postprocess.h"
#include "core/logger.h"
#include "core/exception.h"

#include <cmath>
#include <algorithm>

namespace gis_ai {

std::vector<uint8_t> Postprocess::SigmoidArgmax(const std::vector<float>& output_data,
                                                  int64_t height, int64_t width, int64_t num_classes) {
    size_t total_pixels = static_cast<size_t>(height) * width;
    std::vector<uint8_t> result(total_pixels, 0);

    for (size_t i = 0; i < total_pixels; ++i) {
        float max_val = -1e30f;
        uint8_t max_class = 0;

        for (int64_t c = 0; c < num_classes; ++c) {
            float logit = output_data[c * total_pixels + i];
            float prob = 1.0f / (1.0f + std::exp(-logit));

            if (prob > max_val) {
                max_val = prob;
                max_class = static_cast<uint8_t>(c);
            }
        }
        result[i] = max_class;
    }

    LOG_INFO("SigmoidArgmax: " + std::to_string(num_classes) + " classes, " +
        std::to_string(total_pixels) + " pixels");
    return result;
}

std::vector<float> Postprocess::Sigmoid(const std::vector<float>& logits) {
    std::vector<float> result(logits.size());
    for (size_t i = 0; i < logits.size(); ++i) {
        result[i] = 1.0f / (1.0f + std::exp(-logits[i]));
    }
    return result;
}

std::vector<uint8_t> Postprocess::ThresholdMask(const std::vector<float>& mask, float threshold) {
    std::vector<uint8_t> result(mask.size());
    for (size_t i = 0; i < mask.size(); ++i) {
        result[i] = (mask[i] >= threshold) ? 1 : 0;
    }
    return result;
}

RasterData Postprocess::MaskToRaster(const std::vector<uint8_t>& mask, int width, int height,
                                       const double* geotransform, const std::string& projection) {
    RasterData raster;
    raster.width = width;
    raster.height = height;
    raster.band_count = 1;
    raster.projection = projection;
    memcpy(raster.geotransform, geotransform, sizeof(double) * 6);

    raster.bands.resize(1);
    raster.bands[0].resize(mask.size());
    for (size_t i = 0; i < mask.size(); ++i) {
        raster.bands[0][i] = static_cast<float>(mask[i]);
    }

    raster.band_infos.resize(1);
    raster.band_infos[0].data_type = RasterDataType::Byte;

    return raster;
}

} // namespace gis_ai
