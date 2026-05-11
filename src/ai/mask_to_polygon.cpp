#include "ai/mask_to_polygon.h"
#include "core/logger.h"
#include "core/exception.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace gis_ai {

static bool IsTarget(const std::vector<uint8_t>& mask, int width, int height, int x, int y, uint8_t target_class) {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return mask[static_cast<size_t>(y) * width + x] == target_class;
}

struct Ring {
    std::vector<ContourPoint> points;
    bool is_outer = true;
};

struct PointHash {
    size_t operator()(const std::pair<int,int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 16);
    }
};

static Ring TraceContour8Dir(const std::vector<uint8_t>& mask, int width, int height,
                               int start_x, int start_y, uint8_t target_class,
                               int from_dir) {
    static const int dx8[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    static const int dy8[8] = {0, 1, 1, 1, 0, -1, -1, -1};

    Ring ring;
    ring.is_outer = true;

    int cx = start_x, cy = start_y;
    int dir = from_dir;
    int max_steps = width * height * 2;
    int steps = 0;

    do {
        ring.points.push_back({static_cast<double>(cx), static_cast<double>(cy)});

        int search_dir = (dir + 6) % 8;

        bool found = false;
        for (int d = 0; d < 8; ++d) {
            int nd = (search_dir + d) % 8;
            int nx = cx + dx8[nd];
            int ny = cy + dy8[nd];

            if (IsTarget(mask, width, height, nx, ny, target_class)) {
                cx = nx;
                cy = ny;
                dir = nd;
                found = true;
                break;
            }
        }

        if (!found) break;

        steps++;
        if (steps > max_steps) break;

    } while (cx != start_x || cy != start_y);

    if (ring.points.size() >= 3) {
        ring.points.push_back(ring.points[0]);
    }

    return ring;
}

static double SignedArea(const std::vector<ContourPoint>& pts) {
    if (pts.size() < 3) return 0.0;
    double area = 0.0;
    for (size_t i = 0; i < pts.size() - 1; ++i) {
        area += pts[i].x * pts[i + 1].y;
        area -= pts[i + 1].x * pts[i].y;
    }
    return area / 2.0;
}

static bool PointInRingBBox(const Ring& ring, double px, double py) {
    if (ring.points.size() < 4) return false;
    double min_x = ring.points[0].x, max_x = ring.points[0].x;
    double min_y = ring.points[0].y, max_y = ring.points[0].y;
    for (const auto& pt : ring.points) {
        min_x = std::min(min_x, pt.x);
        max_x = std::max(max_x, pt.x);
        min_y = std::min(min_y, pt.y);
        max_y = std::max(max_y, pt.y);
    }
    return px >= min_x && px <= max_x && py >= min_y && py <= max_y;
}

static bool PointInRingWinding(const Ring& ring, double px, double py) {
    if (!PointInRingBBox(ring, px, py)) return false;

    int winding = 0;
    size_t n = ring.points.size() - 1;
    for (size_t i = 0; i < n; ++i) {
        double y0 = ring.points[i].y, y1 = ring.points[i + 1].y;
        double x0 = ring.points[i].x, x1 = ring.points[i + 1].x;
        if (y0 <= py) {
            if (y1 > py && (x1 - x0) * (py - y0) - (px - x0) * (y1 - y0) > 0) {
                winding++;
            }
        } else {
            if (y1 <= py && (x1 - x0) * (py - y0) - (px - x0) * (y1 - y0) < 0) {
                winding--;
            }
        }
    }
    return winding != 0;
}

static std::vector<Ring> ExtractAllRings(const std::vector<uint8_t>& mask, int width, int height, uint8_t target_class) {
    std::vector<Ring> rings;
    std::unordered_set<std::pair<int,int>, PointHash> traced_starts;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!IsTarget(mask, width, height, x, y, target_class)) continue;

            bool left_not_target = !IsTarget(mask, width, height, x - 1, y, target_class);
            if (!left_not_target) continue;

            if (traced_starts.count({x, y})) continue;

            Ring ring = TraceContour8Dir(mask, width, height, x, y, target_class, 0);
            if (ring.points.size() < 4) continue;

            double area = SignedArea(ring.points);
            ring.is_outer = (area > 0);

            for (const auto& pt : ring.points) {
                int ix = static_cast<int>(pt.x);
                int iy = static_cast<int>(pt.y);
                if (IsTarget(mask, width, height, ix - 1, iy, target_class) == false) {
                    traced_starts.insert({ix, iy});
                }
            }

            rings.push_back(std::move(ring));
        }
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!IsTarget(mask, width, height, x, y, target_class)) continue;
            if (IsTarget(mask, width, height, x - 1, y, target_class)) continue;

            bool right_is_target = IsTarget(mask, width, height, x + 1, y, target_class);
            if (!right_is_target) continue;

            bool is_inner_boundary = true;
            for (const auto& r : rings) {
                if (r.is_outer && PointInRingWinding(r, static_cast<double>(x), static_cast<double>(y))) {
                    is_inner_boundary = true;
                    break;
                }
            }

            if (traced_starts.count({x, y})) continue;

            Ring ring = TraceContour8Dir(mask, width, height, x, y, target_class, 4);
            if (ring.points.size() < 4) continue;

            double area = SignedArea(ring.points);
            if (area >= 0) continue;

            ring.is_outer = false;

            for (const auto& pt : ring.points) {
                int ix = static_cast<int>(pt.x);
                int iy = static_cast<int>(pt.y);
                traced_starts.insert({ix, iy});
            }

            rings.push_back(std::move(ring));
        }
    }

    return rings;
}

static std::vector<std::pair<size_t, std::vector<size_t>>> AssociateHoles(const std::vector<Ring>& rings) {
    std::vector<size_t> outer_indices;
    std::vector<size_t> inner_indices;

    for (size_t i = 0; i < rings.size(); ++i) {
        if (rings[i].is_outer) {
            outer_indices.push_back(i);
        } else {
            inner_indices.push_back(i);
        }
    }

    std::vector<std::pair<size_t, std::vector<size_t>>> result;
    for (size_t oi : outer_indices) {
        result.push_back({oi, {}});
    }

    for (size_t ii : inner_indices) {
        const auto& hole = rings[ii];
        double hx = 0, hy = 0;
        if (!hole.points.empty()) {
            hx = hole.points[0].x;
            hy = hole.points[0].y;
        }

        int best_outer = -1;
        int best_nesting = -1;

        for (size_t k = 0; k < outer_indices.size(); ++k) {
            size_t oi = outer_indices[k];
            if (PointInRingWinding(rings[oi], hx, hy)) {
                int nesting = 0;
                for (size_t m = 0; m < outer_indices.size(); ++m) {
                    if (m == k) continue;
                    if (PointInRingWinding(rings[outer_indices[m]], hx, hy)) {
                        nesting++;
                    }
                }
                if (nesting > best_nesting) {
                    best_nesting = nesting;
                    best_outer = static_cast<int>(k);
                }
            }
        }

        if (best_outer >= 0) {
            result[best_outer].second.push_back(ii);
        }
    }

    return result;
}

std::unique_ptr<VectorData> MaskToPolygon::Execute(const std::vector<uint8_t>& mask, int width, int height,
                                                     const double* geotransform, uint8_t target_class) {
    if (mask.size() != static_cast<size_t>(width) * height) {
        throw GisAiAlgorithmException("Mask size does not match width*height", "MaskToPolygon::Execute");
    }

    auto rings = ExtractAllRings(mask, width, height, target_class);
    auto associations = AssociateHoles(rings);

    auto result = std::make_unique<VectorData>();
    result->feature_type = FeatureType::Polygon;

    for (const auto& [outer_idx, hole_indices] : associations) {
        const auto& outer_ring = rings[outer_idx];
        Feature feature;
        feature.type = FeatureType::Polygon;

        for (const auto& pt : outer_ring.points) {
            double gx = geotransform[0] + pt.x * geotransform[1] + pt.y * geotransform[2];
            double gy = geotransform[3] + pt.x * geotransform[4] + pt.y * geotransform[5];
            feature.coordinates.push_back({gx, gy, 0.0});
        }

        for (size_t hi : hole_indices) {
            const auto& hole_ring = rings[hi];
            std::vector<Coordinate> hole_coords;
            for (const auto& pt : hole_ring.points) {
                double gx = geotransform[0] + pt.x * geotransform[1] + pt.y * geotransform[2];
                double gy = geotransform[3] + pt.x * geotransform[4] + pt.y * geotransform[5];
                hole_coords.push_back({gx, gy, 0.0});
            }
            feature.inner_rings.push_back(std::move(hole_coords));
        }

        if (feature.coordinates.size() >= 4) {
            result->features.push_back(std::move(feature));
        }
    }

    int hole_count = 0;
    for (const auto& [_, holes] : associations) {
        hole_count += static_cast<int>(holes.size());
    }

    LOG_INFO("MaskToPolygon: " + std::to_string(result->features.size()) +
        " polygons extracted (" + std::to_string(hole_count) + " holes)");
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
