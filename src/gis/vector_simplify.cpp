#include "gis/vector_simplify.h"
#include "gis/geos_utils.h"
#include "core/logger.h"
#include "core/exception.h"

namespace gis_ai {

std::unique_ptr<VectorData> VectorSimplify::Execute(const VectorData& input, double tolerance) {
    if (input.features.empty()) {
        throw GisAiAlgorithmException("Input vector data has no features", "VectorSimplify::Execute");
    }
    if (tolerance < 0.0) {
        throw GisAiAlgorithmException("Tolerance must be non-negative", "VectorSimplify::Execute");
    }

    geos_utils::GeosContext geos_ctx;
    if (!geos_ctx.Get()) {
        throw GisAiAlgorithmException("Failed to create GEOS context", "VectorSimplify::Execute");
    }
    GEOSContextHandle_t ctx = geos_ctx.Get();

    auto result = std::make_unique<VectorData>();
    result->feature_type = input.feature_type;
    result->projection = input.projection;

    for (size_t i = 0; i < input.features.size(); ++i) {
        const auto& feature = input.features[i];

        if (feature.type == FeatureType::Point) {
            result->features.push_back(feature);
            continue;
        }

        GEOSGeometry* geom = geos_utils::FeatureToGeos(ctx, feature);
        if (!geom) {
            LOG_WARN("Failed to convert feature " + std::to_string(i) + " to GEOS geometry, skipping");
            continue;
        }

        GEOSGeometry* simplified = GEOSSimplify_r(ctx, geom, tolerance);
        if (!simplified) {
            LOG_WARN("Simplify operation failed for feature " + std::to_string(i));
            GEOSGeom_destroy_r(ctx, geom);
            continue;
        }

        if (GEOSisEmpty_r(ctx, simplified) == 1) {
            GEOSGeom_destroy_r(ctx, simplified);
            GEOSGeom_destroy_r(ctx, geom);
            continue;
        }

        auto new_feature = geos_utils::GeosToFeature(ctx, simplified, feature.type);
        new_feature->attributes = feature.attributes;
        result->features.push_back(std::move(*new_feature));

        GEOSGeom_destroy_r(ctx, simplified);
        GEOSGeom_destroy_r(ctx, geom);
    }

    size_t input_points = 0, output_points = 0;
    for (const auto& f : input.features) input_points += f.coordinates.size();
    for (const auto& f : result->features) output_points += f.coordinates.size();

    LOG_INFO("Simplify operation completed: " + std::to_string(input_points) +
        " input points -> " + std::to_string(output_points) +
        " output points (tolerance=" + std::to_string(tolerance) + ")");
    return result;
}

} // namespace gis_ai
