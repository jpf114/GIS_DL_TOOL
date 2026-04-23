#include "ai/mask_to_polygon.h"
#include "core/logger.h"
#include "core/exception.h"

#include <vector>
#include <algorithm>

namespace gis_ai {

static std::vector<std::vector<ContourPoint>> TraceContours(const std::vector<uint8_t>& mask, int width, int height, uint8_t target_class) {
    std::vector<std::vector<bool>> visited(static_cast<size_t>(height), std::vector<bool>(width, false));
    std::vector<std::vector<ContourPoint>> contours;

    int dx[4] = {1, 0, -1, 0};
    int dy[4] = {0, 1, 0, -1};

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (visited[y][x]) continue;
            if (mask[y * width + x] != target_class) continue;

            bool is_boundary = (x == 0 || y == 0 || x == width - 1 || y == height - 1);
            if (!is_boundary) {
                for (int d = 0; d < 4; ++d) {
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    if (mask[ny * width + nx] != target_class) {
                        is_boundary = true;
                        break;
                    }
                }
            }

            if (!is_boundary) continue;

            std::vector<ContourPoint> contour;
            int cx = x, cy = y;
            int dir = 0;

            do {
                contour.push_back({static_cast<double>(cx), static_cast<double>(cy)});
                visited[cy][cx] = true;

                bool found = false;
                for (int d = 0; d < 4; ++d) {
                    int nd = (dir + d) % 4;
                    int nx = cx + dx[nd];
                    int ny = cy + dy[nd];

                    if (nx >= 0 && nx < width && ny >= 0 && ny < height &&
                        mask[ny * width + nx] == target_class) {
                        cx = nx;
                        cy = ny;
                        dir = (nd + 3) % 4;
                        found = true;
                        break;
                    }
                }

                if (!found) break;
            } while (cx != x || cy != y);

            if (contour.size() >= 3) {
                contour.push_back(contour[0]);
                contours.push_back(std::move(contour));
            }
        }
    }

    return contours;
}

std::unique_ptr<VectorData> MaskToPolygon::Execute(const std::vector<uint8_t>& mask, int width, int height,
                                                     const double* geotransform, uint8_t target_class) {
    if (mask.size() != static_cast<size_t>(width) * height) {
        throw GisAiAlgorithmException("Mask size does not match width*height", "MaskToPolygon::Execute");
    }

    auto contours = TraceContours(mask, width, height, target_class);

    auto result = std::make_unique<VectorData>();
    result->feature_type = FeatureType::Polygon;

    for (auto& contour : contours) {
        Feature feature;
        feature.type = FeatureType::Polygon;

        for (auto& pt : contour) {
            double gx = geotransform[0] + pt.x * geotransform[1] + pt.y * geotransform[2];
            double gy = geotransform[3] + pt.x * geotransform[4] + pt.y * geotransform[5];
            feature.coordinates.push_back({gx, gy, 0.0});
        }

        if (feature.coordinates.size() >= 4) {
            result->features.push_back(std::move(feature));
        }
    }

    LOG_INFO("MaskToPolygon: " + std::to_string(contours.size()) + " polygons extracted");
    return result;
}

std::unique_ptr<VectorData> MaskToPolygon::ExecuteFromRaster(const RasterData& raster, uint8_t target_class) {
    if (raster.bands.empty()) {
        throw GisAiAlgorithmException("Raster has no bands", "MaskToPolygon::ExecuteFromRaster");
    }

    std::vector<uint8_t> mask(raster.bands[0].size());
    for (size_t i = 0; i < mask.size(); ++i) {
        mask[i] = static_cast<uint8_t>(std::round(raster.bands[0][i]));
    }

    return Execute(mask, raster.width, raster.height, raster.geotransform, target_class);
}

} // namespace gis_ai
