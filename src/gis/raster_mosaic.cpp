#include "gis/raster_mosaic.h"
#include "core/logger.h"
#include "core/exception.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gis_ai {

std::unique_ptr<RasterData> RasterMosaic::Execute(const std::vector<std::reference_wrapper<const RasterData>>& rasters) {
    if (rasters.empty()) {
        throw GisAiAlgorithmException("No input rasters provided", "RasterMosaic::Execute");
    }
    if (rasters.size() == 1) {
        const auto& src = rasters[0].get();
        auto result = std::make_unique<RasterData>();
        result->width = src.width;
        result->height = src.height;
        result->band_count = src.band_count;
        result->projection = src.projection;
        memcpy(result->geotransform, src.geotransform, sizeof(double) * 6);
        result->bands = src.bands;
        return result;
    }

    int band_count = rasters[0].get().band_count;
    for (size_t i = 1; i < rasters.size(); ++i) {
        if (rasters[i].get().band_count != band_count) {
            throw GisAiAlgorithmException("All rasters must have the same number of bands",
                "RasterMosaic::Execute");
        }
    }

    double global_min_x = std::numeric_limits<double>::max();
    double global_max_y = std::numeric_limits<double>::lowest();
    double global_max_x = std::numeric_limits<double>::lowest();
    double global_min_y = std::numeric_limits<double>::max();

    double ref_pixel_size_x = 0.0;
    double ref_pixel_size_y = 0.0;
    bool first = true;

    for (const auto& raster_ref : rasters) {
        const auto& r = raster_ref.get();
        const double* gt = r.geotransform;

        double min_x = gt[0];
        double max_y = gt[3];
        double max_x = gt[0] + r.width * gt[1] + r.height * gt[2];
        double min_y = gt[3] + r.width * gt[4] + r.height * gt[5];

        global_min_x = std::min(global_min_x, min_x);
        global_max_y = std::max(global_max_y, max_y);
        global_max_x = std::max(global_max_x, max_x);
        global_min_y = std::min(global_min_y, min_y);

        if (first) {
            ref_pixel_size_x = std::abs(gt[1]);
            ref_pixel_size_y = std::abs(gt[5]);
            first = false;
        }
    }

    if (ref_pixel_size_x < 1e-15 || ref_pixel_size_y < 1e-15) {
        throw GisAiAlgorithmException("Invalid pixel size in geotransform", "RasterMosaic::Execute");
    }

    int mosaic_width = static_cast<int>(std::ceil((global_max_x - global_min_x) / ref_pixel_size_x));
    int mosaic_height = static_cast<int>(std::ceil((global_max_y - global_min_y) / ref_pixel_size_y));

    mosaic_width = std::max(1, mosaic_width);
    mosaic_height = std::max(1, mosaic_height);

    auto result = std::make_unique<RasterData>();
    result->width = mosaic_width;
    result->height = mosaic_height;
    result->band_count = band_count;
    result->projection = rasters[0].get().projection;

    result->geotransform[0] = global_min_x;
    result->geotransform[1] = ref_pixel_size_x;
    result->geotransform[2] = 0.0;
    result->geotransform[3] = global_max_y;
    result->geotransform[4] = 0.0;
    result->geotransform[5] = -ref_pixel_size_y;

    size_t total = static_cast<size_t>(mosaic_width) * mosaic_height;
    result->bands.resize(band_count);
    for (int b = 0; b < band_count; ++b) {
        result->bands[b].resize(total, std::numeric_limits<float>::quiet_NaN());
    }

    for (const auto& raster_ref : rasters) {
        const auto& r = raster_ref.get();
        const double* gt = r.geotransform;

        int off_x = static_cast<int>(std::round((gt[0] - global_min_x) / ref_pixel_size_x));
        int off_y = static_cast<int>(std::round((global_max_y - gt[3]) / ref_pixel_size_y));

        for (int b = 0; b < band_count; ++b) {
            for (int ry = 0; ry < r.height; ++ry) {
                int dst_y = off_y + ry;
                if (dst_y < 0 || dst_y >= mosaic_height) continue;

                for (int rx = 0; rx < r.width; ++rx) {
                    int dst_x = off_x + rx;
                    if (dst_x < 0 || dst_x >= mosaic_width) continue;

                    float val = r.bands[b][ry * r.width + rx];
                    if (!std::isnan(val)) {
                        result->bands[b][dst_y * mosaic_width + dst_x] = val;
                    }
                }
            }
        }
    }

    LOG_INFO("Mosaic completed: " + std::to_string(rasters.size()) + " rasters -> " +
        std::to_string(mosaic_width) + "x" + std::to_string(mosaic_height));
    return result;
}

} // namespace gis_ai
