#include "gis/vector_intersect.h"
#include "gis/geos_utils.h"
#include "core/logger.h"
#include "core/exception.h"

namespace gis_ai {

bool VectorIntersect::Intersects(const VectorData& a, const VectorData& b) {
    geos_utils::GeosContext geos_ctx;
    if (!geos_ctx.Get()) return false;
    GEOSContextHandle_t ctx = geos_ctx.Get();

    for (const auto& feat_a : a.features) {
        GEOSGeometry* geom_a = geos_utils::FeatureToGeos(ctx, feat_a);
        if (!geom_a) continue;

        for (const auto& feat_b : b.features) {
            GEOSGeometry* geom_b = geos_utils::FeatureToGeos(ctx, feat_b);
            if (!geom_b) continue;

            char intersects = GEOSIntersects_r(ctx, geom_a, geom_b);
            GEOSGeom_destroy_r(ctx, geom_b);

            if (intersects == 1) {
                GEOSGeom_destroy_r(ctx, geom_a);
                return true;
            }
        }
        GEOSGeom_destroy_r(ctx, geom_a);
    }

    return false;
}

std::unique_ptr<VectorData> VectorIntersect::Execute(const VectorData& a, const VectorData& b) {
    if (a.features.empty() || b.features.empty()) {
        throw GisAiAlgorithmException("Input vector data has no features", "VectorIntersect::Execute");
    }

    geos_utils::GeosContext geos_ctx;
    if (!geos_ctx.Get()) {
        throw GisAiAlgorithmException("Failed to create GEOS context", "VectorIntersect::Execute");
    }
    GEOSContextHandle_t ctx = geos_ctx.Get();

    auto result = std::make_unique<VectorData>();
    result->projection = a.projection;

    for (const auto& feat_a : a.features) {
        GEOSGeometry* geom_a = geos_utils::FeatureToGeos(ctx, feat_a);
        if (!geom_a) continue;

        for (const auto& feat_b : b.features) {
            GEOSGeometry* geom_b = geos_utils::FeatureToGeos(ctx, feat_b);
            if (!geom_b) continue;

            GEOSGeometry* intersection = GEOSIntersection_r(ctx, geom_a, geom_b);
            if (!intersection) {
                GEOSGeom_destroy_r(ctx, geom_b);
                continue;
            }

            if (GEOSisEmpty_r(ctx, intersection) == 1) {
                GEOSGeom_destroy_r(ctx, intersection);
                GEOSGeom_destroy_r(ctx, geom_b);
                continue;
            }

            int geom_type = GEOSGeomTypeId_r(ctx, intersection);
            FeatureType result_type = geos_utils::GeosTypeToFeatureType(geom_type);

            geos_utils::ExtractGeosFeatures(ctx, intersection, result_type,
                feat_a.attributes, result->features);

            GEOSGeom_destroy_r(ctx, intersection);
            GEOSGeom_destroy_r(ctx, geom_b);
        }
        GEOSGeom_destroy_r(ctx, geom_a);
    }

    if (!result->features.empty()) {
        result->feature_type = result->features[0].type;
    }

    LOG_INFO("Intersect operation completed: " + std::to_string(result->features.size()) + " intersection features");
    return result;
}

} // namespace gis_ai
