#include "gis/raster_clip.h"
#include "core/logger.h"
#include "core/exception.h"

#include <algorithm>
#include <cmath>

namespace gis_ai {

std::unique_ptr<RasterData> RasterClip::Execute(const RasterData& input, const BoundingBox& bbox) {
    if (input.width <= 0 || input.height <= 0) {
        throw GisAiAlgorithmException("Input raster has invalid dimensions", "RasterClip::Execute");
    }

    double gt[6];
    memcpy(gt, input.geotransform, sizeof(double) * 6);

    double inv_det = gt[1] * gt[5] - gt[2] * gt[4];
    if (std::abs(inv_det) < 1e-15) {
        throw GisAiAlgorithmException("Invalid geotransform (degenerate)", "RasterClip::Execute");
    }

    auto PixelToGeo = [&](int px, int py, double& gx, double& gy) {
        gx = gt[0] + px * gt[1] + py * gt[2];
        gy = gt[3] + px * gt[4] + py * gt[5];
    };

    auto GeoToPixel = [&](double gx, double gy, int& px, int& py) {
        double dx = gx - gt[0];
        double dy = gy - gt[3];
        px = static_cast<int>(std::round((dx * gt[5] - dy * gt[2]) / inv_det));
        py = static_cast<int>(std::round((dy * gt[1] - dx * gt[4]) / inv_det));
    };

    int x_min, y_min, x_max, y_max;
    GeoToPixel(bbox.min_x, bbox.max_y, x_min, y_min);
    GeoToPixel(bbox.max_x, bbox.min_y, x_max, y_max);

    x_min = std::max(0, x_min);
    y_min = std::max(0, y_min);
    x_max = std::min(input.width - 1, x_max);
    y_max = std::min(input.height - 1, y_max);

    if (x_min > x_max || y_min > y_max) {
        throw GisAiAlgorithmException("Bounding box does not intersect with raster", "RasterClip::Execute");
    }

    int clip_width = x_max - x_min + 1;
    int clip_height = y_max - y_min + 1;

    return ExecuteByPixel(input, x_min, y_min, clip_width, clip_height);
}

std::unique_ptr<RasterData> RasterClip::ExecuteByPixel(const RasterData& input, int x_off, int y_off, int width, int height) {
    if (x_off < 0 || y_off < 0 || width <= 0 || height <= 0) {
        throw GisAiAlgorithmException("Invalid clip parameters", "RasterClip::ExecuteByPixel");
    }
    if (x_off + width > input.width || y_off + height > input.height) {
        throw GisAiAlgorithmException("Clip region exceeds raster bounds", "RasterClip::ExecuteByPixel");
    }

    auto result = std::make_unique<RasterData>();
    result->width = width;
    result->height = height;
    result->band_count = input.band_count;
    result->projection = input.projection;
    result->band_infos = input.band_infos;

    result->geotransform[0] = input.geotransform[0] + x_off * input.geotransform[1] + y_off * input.geotransform[2];
    result->geotransform[1] = input.geotransform[1];
    result->geotransform[2] = input.geotransform[2];
    result->geotransform[3] = input.geotransform[3] + x_off * input.geotransform[4] + y_off * input.geotransform[5];
    result->geotransform[4] = input.geotransform[4];
    result->geotransform[5] = input.geotransform[5];

    result->bands.resize(input.band_count);

    for (int b = 0; b < input.band_count; ++b) {
        result->bands[b].resize(static_cast<size_t>(width) * height);
        for (int y = 0; y < height; ++y) {
            const float* src_row = &input.bands[b][(y_off + y) * input.width + x_off];
            float* dst_row = &result->bands[b][y * width];
            memcpy(dst_row, src_row, sizeof(float) * width);
        }
    }

    LOG_INFO("Clip completed: offset(" + std::to_string(x_off) + "," + std::to_string(y_off) +
        ") size " + std::to_string(width) + "x" + std::to_string(height));
    return result;
}

} // namespace gis_ai
