#include "gis/raster_resample.h"
#include "core/logger.h"
#include "core/exception.h"

#include <cmath>
#include <algorithm>

namespace gis_ai {

std::unique_ptr<RasterData> RasterResample::Execute(const RasterData& input, int new_width, int new_height,
                                                      ResampleMethod method) {
    if (new_width <= 0 || new_height <= 0) {
        throw GisAiAlgorithmException("Target dimensions must be positive", "RasterResample::Execute");
    }
    if (input.width <= 0 || input.height <= 0) {
        throw GisAiAlgorithmException("Input raster has invalid dimensions", "RasterResample::Execute");
    }

    auto result = std::make_unique<RasterData>();
    result->width = new_width;
    result->height = new_height;
    result->band_count = input.band_count;
    result->projection = input.projection;

    double x_scale = static_cast<double>(input.width) / new_width;
    double y_scale = static_cast<double>(input.height) / new_height;

    result->geotransform[0] = input.geotransform[0];
    result->geotransform[1] = input.geotransform[1] * x_scale;
    result->geotransform[2] = input.geotransform[2];
    result->geotransform[3] = input.geotransform[3];
    result->geotransform[4] = input.geotransform[4];
    result->geotransform[5] = input.geotransform[5] * y_scale;

    result->bands.resize(input.band_count);

    for (int b = 0; b < input.band_count; ++b) {
        result->bands[b].resize(static_cast<size_t>(new_width) * new_height);

        for (int y = 0; y < new_height; ++y) {
            for (int x = 0; x < new_width; ++x) {
                double src_x = (x + 0.5) * x_scale - 0.5;
                double src_y = (y + 0.5) * y_scale - 0.5;

                float value = 0.0f;

                if (method == ResampleMethod::Nearest) {
                    int px = std::max(0, std::min(input.width - 1, static_cast<int>(std::round(src_x))));
                    int py = std::max(0, std::min(input.height - 1, static_cast<int>(std::round(src_y))));
                    value = input.bands[b][py * input.width + px];
                } else if (method == ResampleMethod::Bilinear) {
                    int x0 = std::max(0, static_cast<int>(std::floor(src_x)));
                    int y0 = std::max(0, static_cast<int>(std::floor(src_y)));
                    int x1 = std::min(input.width - 1, x0 + 1);
                    int y1 = std::min(input.height - 1, y0 + 1);

                    double fx = src_x - std::floor(src_x);
                    double fy = src_y - std::floor(src_y);
                    fx = std::max(0.0, std::min(1.0, fx));
                    fy = std::max(0.0, std::min(1.0, fy));

                    float v00 = input.bands[b][y0 * input.width + x0];
                    float v10 = input.bands[b][y0 * input.width + x1];
                    float v01 = input.bands[b][y1 * input.width + x0];
                    float v11 = input.bands[b][y1 * input.width + x1];

                    value = static_cast<float>(
                        v00 * (1 - fx) * (1 - fy) +
                        v10 * fx * (1 - fy) +
                        v01 * (1 - fx) * fy +
                        v11 * fx * fy
                    );
                }

                result->bands[b][y * new_width + x] = value;
            }
        }
    }

    LOG_INFO("Resample completed: " + std::to_string(input.width) + "x" + std::to_string(input.height) +
        " -> " + std::to_string(new_width) + "x" + std::to_string(new_height));
    return result;
}

} // namespace gis_ai
