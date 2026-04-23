#include "gis/vector_topology.h"
#include "gis/geos_utils.h"
#include "core/logger.h"
#include "core/exception.h"

namespace gis_ai {

std::vector<TopologyIssue> VectorTopology::CheckOverlaps(const VectorData& data) {
    std::vector<TopologyIssue> issues;

    if (data.features.size() < 2) return issues;
    if (data.feature_type != FeatureType::Polygon) {
        LOG_WARN("Overlap check is meaningful only for polygon data");
        return issues;
    }

    geos_utils::GeosContext geos_ctx;
    if (!geos_ctx.Get()) return issues;
    GEOSContextHandle_t ctx = geos_ctx.Get();

    std::vector<GEOSGeometry*> geoms;
    for (const auto& feat : data.features) {
        GEOSGeometry* g = geos_utils::FeatureToGeos(ctx, feat);
        geoms.push_back(g);
    }

    for (size_t i = 0; i < geoms.size(); ++i) {
        if (!geoms[i]) continue;
        for (size_t j = i + 1; j < geoms.size(); ++j) {
            if (!geoms[j]) continue;

            char intersects = GEOSIntersects_r(ctx, geoms[i], geoms[j]);
            if (intersects != 1) continue;

            GEOSGeometry* intersection = GEOSIntersection_r(ctx, geoms[i], geoms[j]);
            if (!intersection) continue;

            if (GEOSisEmpty_r(ctx, intersection) == 0) {
                int geom_type = GEOSGeomTypeId_r(ctx, intersection);
                if (geom_type == GEOS_POLYGON || geom_type == GEOS_MULTIPOLYGON) {
                    double area = 0.0;
                    GEOSArea_r(ctx, intersection, &area);

                    TopologyIssue issue;
                    issue.feature_index = static_cast<int>(i);
                    issue.issue_type = "overlap";
                    issue.description = "Feature " + std::to_string(i) +
                        " overlaps with feature " + std::to_string(j) +
                        " (area=" + std::to_string(area) + ")";
                    issues.push_back(issue);
                }
            }
            GEOSGeom_destroy_r(ctx, intersection);
        }
    }

    for (auto* g : geoms) {
        if (g) GEOSGeom_destroy_r(ctx, g);
    }

    LOG_INFO("Overlap check completed: " + std::to_string(issues.size()) + " overlaps found");
    return issues;
}

std::vector<TopologyIssue> VectorTopology::CheckGaps(const VectorData& data, double tolerance) {
    std::vector<TopologyIssue> issues;

    if (data.features.size() < 2) return issues;
    if (data.feature_type != FeatureType::Polygon) {
        LOG_WARN("Gap check is meaningful only for polygon data");
        return issues;
    }

    geos_utils::GeosContext geos_ctx;
    if (!geos_ctx.Get()) return issues;
    GEOSContextHandle_t ctx = geos_ctx.Get();

    std::vector<GEOSGeometry*> geoms;
    for (const auto& feat : data.features) {
        GEOSGeometry* g = geos_utils::FeatureToGeos(ctx, feat);
        geoms.push_back(g);
    }

    if (geoms.empty()) return issues;

    GEOSGeometry* union_geom = GEOSGeom_clone_r(ctx, geoms[0]);
    for (size_t i = 1; i < geoms.size(); ++i) {
        if (!geoms[i]) continue;
        GEOSGeometry* new_union = GEOSUnion_r(ctx, union_geom, geoms[i]);
        GEOSGeom_destroy_r(ctx, union_geom);
        if (!new_union) {
            for (auto* g : geoms) {
                if (g) GEOSGeom_destroy_r(ctx, g);
            }
            return issues;
        }
        union_geom = new_union;
    }

    GEOSGeometry* envelope = GEOSEnvelope_r(ctx, union_geom);
    if (envelope) {
        double env_area = 0.0, union_area = 0.0;
        GEOSArea_r(ctx, envelope, &env_area);
        GEOSArea_r(ctx, union_geom, &union_area);

        double gap_area = env_area - union_area;
        if (gap_area > tolerance) {
            GEOSGeometry* gap = GEOSDifference_r(ctx, envelope, union_geom);
            if (gap && GEOSisEmpty_r(ctx, gap) == 0) {
                TopologyIssue issue;
                issue.feature_index = -1;
                issue.issue_type = "gap";
                issue.description = "Gap detected between features (area=" +
                    std::to_string(gap_area) + ")";
                issues.push_back(issue);
            }
            if (gap) GEOSGeom_destroy_r(ctx, gap);
        }
        GEOSGeom_destroy_r(ctx, envelope);
    }

    GEOSGeom_destroy_r(ctx, union_geom);
    for (auto* g : geoms) {
        if (g) GEOSGeom_destroy_r(ctx, g);
    }

    LOG_INFO("Gap check completed: " + std::to_string(issues.size()) + " gaps found");
    return issues;
}

std::vector<TopologyIssue> VectorTopology::CheckValidity(const VectorData& data) {
    std::vector<TopologyIssue> issues;

    geos_utils::GeosContext geos_ctx;
    if (!geos_ctx.Get()) return issues;
    GEOSContextHandle_t ctx = geos_ctx.Get();

    for (size_t i = 0; i < data.features.size(); ++i) {
        GEOSGeometry* geom = geos_utils::FeatureToGeos(ctx, data.features[i]);
        if (!geom) {
            TopologyIssue issue;
            issue.feature_index = static_cast<int>(i);
            issue.issue_type = "invalid";
            issue.description = "Feature " + std::to_string(i) +
                " could not be converted to GEOS geometry";
            issues.push_back(issue);
            continue;
        }

        char is_valid = GEOSisValid_r(ctx, geom);
        if (is_valid != 1) {
            char* reason = GEOSisValidReason_r(ctx, geom);
            TopologyIssue issue;
            issue.feature_index = static_cast<int>(i);
            issue.issue_type = "invalid";
            issue.description = "Feature " + std::to_string(i) + " is invalid: " +
                (reason ? std::string(reason) : "unknown reason");
            issues.push_back(issue);
            if (reason) GEOSFree_r(ctx, reason);
        }

        GEOSGeom_destroy_r(ctx, geom);
    }

    LOG_INFO("Validity check completed: " + std::to_string(issues.size()) + " invalid features found");
    return issues;
}

std::vector<TopologyIssue> VectorTopology::FullCheck(const VectorData& data, double tolerance) {
    std::vector<TopologyIssue> issues;

    auto validity_issues = CheckValidity(data);
    issues.insert(issues.end(), validity_issues.begin(), validity_issues.end());

    auto overlap_issues = CheckOverlaps(data);
    issues.insert(issues.end(), overlap_issues.begin(), overlap_issues.end());

    auto gap_issues = CheckGaps(data, tolerance);
    issues.insert(issues.end(), gap_issues.begin(), gap_issues.end());

    LOG_INFO("Full topology check completed: " + std::to_string(issues.size()) + " total issues found");
    return issues;
}

} // namespace gis_ai
