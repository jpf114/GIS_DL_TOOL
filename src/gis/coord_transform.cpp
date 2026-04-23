#include "gis/coord_transform.h"
#include "core/logger.h"
#include "core/exception.h"

#include <proj.h>

namespace gis_ai {

CoordPair CoordTransform::Transform(double x, double y, double z, const std::string& src_crs, const std::string& dst_crs) {
    PJ_CONTEXT* ctx = proj_context_create();
    if (!ctx) {
        throw GisAiAlgorithmException("Failed to create PROJ context", "CoordTransform::Transform");
    }

    PJ* transform = proj_create_crs_to_crs(ctx, src_crs.c_str(), dst_crs.c_str(), nullptr);
    if (!transform) {
        proj_context_destroy(ctx);
        throw GisAiAlgorithmException("Failed to create coordinate transformation from '" +
            src_crs + "' to '" + dst_crs + "'", "CoordTransform::Transform");
    }

    PJ* normalized = proj_normalize_for_visualization(ctx, transform);
    if (normalized) {
        proj_destroy(transform);
        transform = normalized;
    }

    PJ_COORD coord;
    coord.xyz.x = x;
    coord.xyz.y = y;
    coord.xyz.z = z;

    PJ_COORD result_coord = proj_trans(transform, PJ_FWD, coord);

    CoordPair result;
    result.x = result_coord.xyz.x;
    result.y = result_coord.xyz.y;
    result.z = result_coord.xyz.z;

    proj_destroy(transform);
    proj_context_destroy(ctx);

    return result;
}

std::vector<CoordPair> CoordTransform::TransformBatch(const std::vector<CoordPair>& coords,
                                                        const std::string& src_crs, const std::string& dst_crs) {
    if (coords.empty()) {
        return {};
    }

    PJ_CONTEXT* ctx = proj_context_create();
    if (!ctx) {
        throw GisAiAlgorithmException("Failed to create PROJ context", "CoordTransform::TransformBatch");
    }

    PJ* transform = proj_create_crs_to_crs(ctx, src_crs.c_str(), dst_crs.c_str(), nullptr);
    if (!transform) {
        proj_context_destroy(ctx);
        throw GisAiAlgorithmException("Failed to create coordinate transformation from '" +
            src_crs + "' to '" + dst_crs + "'", "CoordTransform::TransformBatch");
    }

    PJ* normalized = proj_normalize_for_visualization(ctx, transform);
    if (normalized) {
        proj_destroy(transform);
        transform = normalized;
    }

    size_t count = coords.size();
    auto* pj_coords = new PJ_COORD[count];
    for (size_t i = 0; i < count; ++i) {
        pj_coords[i].xyz.x = coords[i].x;
        pj_coords[i].xyz.y = coords[i].y;
        pj_coords[i].xyz.z = coords[i].z;
    }

    size_t failed = proj_trans_generic(transform, PJ_FWD,
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

    delete[] pj_coords;
    proj_destroy(transform);
    proj_context_destroy(ctx);

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
