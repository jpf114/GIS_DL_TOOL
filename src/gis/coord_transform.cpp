#include "gis/coord_transform.h"
#include "core/logger.h"
#include "core/exception.h"

#include <proj.h>
#include <mutex>
#include <memory>
#include <unordered_map>

namespace gis_ai {

struct ProjTransformDeleter {
    void operator()(PJ* p) const { if (p) proj_destroy(p); }
};

struct ProjContextDeleter {
    void operator()(PJ_CONTEXT* c) const { if (c) proj_context_destroy(c); }
};

using ProjTransformPtr = std::shared_ptr<PJ>;
using ProjContextPtr = std::unique_ptr<PJ_CONTEXT, ProjContextDeleter>;

struct ProjTransformDeleterForShared {
    void operator()(PJ* p) const { if (p) proj_destroy(p); }
};

class ProjCache {
public:
    static ProjCache& Instance() {
        static ProjCache instance;
        return instance;
    }

    std::shared_ptr<PJ> GetTransform(const std::string& src_crs, const std::string& dst_crs) {
        std::string key = src_crs + " -> " + dst_crs;
        std::lock_guard<std::mutex> lock(mutex_);

        if (!ctx_) {
            ctx_ = ProjContextPtr(proj_context_create());
        }

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            auto locked = it->second.lock();
            if (locked) return locked;
        }

        PJ_CONTEXT* ctx = ctx_.get();
        PJ* transform = proj_create_crs_to_crs(ctx, src_crs.c_str(), dst_crs.c_str(), nullptr);
        if (!transform) {
            throw GisAiAlgorithmException("Failed to create coordinate transformation from '" +
                src_crs + "' to '" + dst_crs + "'", "CoordTransform");
        }

        PJ* normalized = proj_normalize_for_visualization(ctx, transform);
        if (normalized) {
            proj_destroy(transform);
            transform = normalized;
        }

        auto ptr = std::shared_ptr<PJ>(transform, ProjTransformDeleterForShared());
        cache_[key] = ptr;
        return ptr;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        ctx_.reset();
    }

private:
    ProjCache() = default;
    std::mutex mutex_;
    ProjContextPtr ctx_;
    std::unordered_map<std::string, std::weak_ptr<PJ>> cache_;
};

CoordPair CoordTransform::Transform(double x, double y, double z, const std::string& src_crs, const std::string& dst_crs) {
    auto transform = ProjCache::Instance().GetTransform(src_crs, dst_crs);

    PJ_COORD coord;
    coord.xyz.x = x;
    coord.xyz.y = y;
    coord.xyz.z = z;

    PJ_COORD result_coord = proj_trans(transform.get(), PJ_FWD, coord);

    CoordPair result;
    result.x = result_coord.xyz.x;
    result.y = result_coord.xyz.y;
    result.z = result_coord.xyz.z;

    return result;
}

std::vector<CoordPair> CoordTransform::TransformBatch(const std::vector<CoordPair>& coords,
                                                        const std::string& src_crs, const std::string& dst_crs) {
    if (coords.empty()) {
        return {};
    }

    auto transform = ProjCache::Instance().GetTransform(src_crs, dst_crs);

    size_t count = coords.size();
    std::vector<PJ_COORD> pj_coords(count);
    for (size_t i = 0; i < count; ++i) {
        pj_coords[i].xyz.x = coords[i].x;
        pj_coords[i].xyz.y = coords[i].y;
        pj_coords[i].xyz.z = coords[i].z;
    }

    size_t failed = proj_trans_generic(transform.get(), PJ_FWD,
        &(pj_coords[0].xyz.x), sizeof(PJ_COORD), count,
        &(pj_coords[0].xyz.y), sizeof(PJ_COORD), count,
        &(pj_coords[0].xyz.z), sizeof(PJ_COORD), count,
        nullptr, 0, 0);

    std::vector<CoordPair> results(count);
    for (size_t i = 0; i < count; ++i) {
        results[i].x = pj_coords[i].xyz.x;
        results[i].y = pj_coords[i].xyz.y;
        results[i].z = pj_coords[i].xyz.z;
    }

    if (failed > 0) {
        LOG_WARN("Coordinate transformation had " + std::to_string(failed) + " failures");
    }

    LOG_INFO("Batch transform completed: " + std::to_string(count) + " coordinates");
    return results;
}

std::unique_ptr<VectorData> CoordTransform::TransformVector(const VectorData& input, const std::string& dst_crs) {
    if (input.projection.empty()) {
        throw GisAiAlgorithmException("Input vector data has no projection information",
            "CoordTransform::TransformVector");
    }

    auto result = std::make_unique<VectorData>();
    result->feature_type = input.feature_type;
    result->projection = dst_crs;

    std::vector<CoordPair> coords;
    for (const auto& feature : input.features) {
        for (const auto& c : feature.coordinates) {
            coords.push_back({c.x, c.y, c.z});
        }
        for (const auto& ring : feature.inner_rings) {
            for (const auto& c : ring) {
                coords.push_back({c.x, c.y, c.z});
            }
        }
    }

    auto transformed = TransformBatch(coords, input.projection, dst_crs);

    size_t coord_idx = 0;
    for (const auto& feature : input.features) {
        Feature new_feature;
        new_feature.type = feature.type;
        new_feature.attributes = feature.attributes;

        for (size_t i = 0; i < feature.coordinates.size(); ++i) {
            if (coord_idx < transformed.size()) {
                const auto& tc = transformed[coord_idx];
                new_feature.coordinates.push_back({tc.x, tc.y, tc.z});
                coord_idx++;
            }
        }

        for (const auto& ring : feature.inner_rings) {
            std::vector<Coordinate> new_ring;
            for (size_t i = 0; i < ring.size(); ++i) {
                if (coord_idx < transformed.size()) {
                    const auto& tc = transformed[coord_idx];
                    new_ring.push_back({tc.x, tc.y, tc.z});
                    coord_idx++;
                }
            }
            new_feature.inner_rings.push_back(std::move(new_ring));
        }

        result->features.push_back(std::move(new_feature));
    }

    LOG_INFO("Vector transform completed: " + std::to_string(result->features.size()) + " features");
    return result;
}

std::unique_ptr<PointCloudData> CoordTransform::TransformPointCloud(const PointCloudData& input, const std::string& dst_crs) {
    if (input.coordinate_system.empty()) {
        throw GisAiAlgorithmException("Input point cloud has no coordinate system information",
            "CoordTransform::TransformPointCloud");
    }

    std::vector<CoordPair> coords;
    coords.reserve(input.points.size());
    for (const auto& pt : input.points) {
        coords.push_back({pt.x, pt.y, pt.z});
    }

    auto transformed = TransformBatch(coords, input.coordinate_system, dst_crs);

    auto result = std::make_unique<PointCloudData>();
    result->coordinate_system = dst_crs;

    for (size_t i = 0; i < input.points.size(); ++i) {
        Point pt;
        pt.x = transformed[i].x;
        pt.y = transformed[i].y;
        pt.z = transformed[i].z;
        pt.intensity = input.points[i].intensity;
        pt.classification = input.points[i].classification;
        result->points.push_back(pt);
    }

    LOG_INFO("PointCloud transform completed: " + std::to_string(result->points.size()) + " points");
    return result;
}

} // namespace gis_ai
