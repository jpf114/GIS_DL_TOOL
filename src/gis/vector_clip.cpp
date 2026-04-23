#include "gis/vector_clip.h"
#include "gis/geos_utils.h"
#include "core/logger.h"
#include "core/exception.h"

namespace gis_ai {

std::unique_ptr<VectorData> VectorClip::Execute(const VectorData& target, const VectorData& clipper) {
    if (target.features.empty()) {
        throw GisAiAlgorithmException("Target vector data has no features", "VectorClip::Execute");
    }
    if (clipper.features.empty()) {
        throw GisAiAlgorithmException("Clipper vector data has no features", "VectorClip::Execute");
    }

    geos_utils::GeosContext geos_ctx;
    if (!geos_ctx.Get()) {
        throw GisAiAlgorithmException("Failed to create GEOS context", "VectorClip::Execute");
    }
    GEOSContextHandle_t ctx = geos_ctx.Get();

    GEOSGeometry* clipper_union = nullptr;
    if (clipper.features.size() == 1) {
        clipper_union = geos_utils::FeatureToGeos(ctx, clipper.features[0]);
    } else {
        std::vector<GEOSGeometry*> clip_geoms;
        for (const auto& feat : clipper.features) {
            GEOSGeometry* g = geos_utils::FeatureToGeos(ctx, feat);
            if (g) clip_geoms.push_back(g);
        }
        if (!clip_geoms.empty()) {
            clipper_union = GEOSGeom_createCollection_r(ctx, GEOS_MULTIPOLYGON,
                clip_geoms.data(), static_cast<unsigned int>(clip_geoms.size()));
        }
    }

    if (!clipper_union) {
        throw GisAiAlgorithmException("Failed to create clipper geometry", "VectorClip::Execute");
    }

    auto result = std::make_unique<VectorData>();
    result->projection = target.projection;

    for (const auto& feat : target.features) {
        GEOSGeometry* target_geom = geos_utils::FeatureToGeos(ctx, feat);
        if (!target_geom) continue;

        GEOSGeometry* clipped = GEOSIntersection_r(ctx, target_geom, clipper_union);
        if (!clipped) {
            GEOSGeom_destroy_r(ctx, target_geom);
            continue;
        }

        if (GEOSisEmpty_r(ctx, clipped) == 1) {
            GEOSGeom_destroy_r(ctx, clipped);
            GEOSGeom_destroy_r(ctx, target_geom);
            continue;
        }

        int geom_type = GEOSGeomTypeId_r(ctx, clipped);
        FeatureType result_type = geos_utils::GeosTypeToFeatureType(geom_type);

        geos_utils::ExtractGeosFeatures(ctx, clipped, result_type,
            feat.attributes, result->features);

        GEOSGeom_destroy_r(ctx, clipped);
        GEOSGeom_destroy_r(ctx, target_geom);
    }

    if (!result->features.empty()) {
        result->feature_type = result->features[0].type;
    }

    GEOSGeom_destroy_r(ctx, clipper_union);

    LOG_INFO("Clip operation completed: " + std::to_string(target.features.size()) +
        " target features -> " + std::to_string(result->features.size()) + " clipped features");
    return result;
}

} // namespace gis_ai
