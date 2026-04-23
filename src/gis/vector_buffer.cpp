#include "gis/vector_buffer.h"
#include "gis/geos_utils.h"
#include "core/logger.h"
#include "core/exception.h"

namespace gis_ai {

std::unique_ptr<VectorData> VectorBuffer::Execute(const VectorData& input, double distance, int quad_segments) {
    if (input.features.empty()) {
        throw GisAiAlgorithmException("Input vector data has no features", "VectorBuffer::Execute");
    }

    geos_utils::GeosContext geos_ctx;
    if (!geos_ctx.Get()) {
        throw GisAiAlgorithmException("Failed to create GEOS context", "VectorBuffer::Execute");
    }
    GEOSContextHandle_t ctx = geos_ctx.Get();

    auto result = std::make_unique<VectorData>();
    result->feature_type = FeatureType::Polygon;
    result->projection = input.projection;

    for (size_t i = 0; i < input.features.size(); ++i) {
        const auto& feature = input.features[i];

        GEOSGeometry* geom = geos_utils::FeatureToGeos(ctx, feature);
        if (!geom) {
            LOG_WARN("Failed to convert feature " + std::to_string(i) + " to GEOS geometry, skipping");
            continue;
        }

        GEOSGeometry* buffer_geom = GEOSBuffer_r(ctx, geom, distance, quad_segments);
        if (!buffer_geom) {
            LOG_WARN("Buffer operation failed for feature " + std::to_string(i));
            GEOSGeom_destroy_r(ctx, geom);
            continue;
        }

        geos_utils::ExtractGeosFeatures(ctx, buffer_geom, FeatureType::Polygon,
            feature.attributes, result->features);

        GEOSGeom_destroy_r(ctx, buffer_geom);
        GEOSGeom_destroy_r(ctx, geom);
    }

    LOG_INFO("Buffer operation completed: " + std::to_string(input.features.size()) +
        " input features -> " + std::to_string(result->features.size()) +
        " output features (distance=" + std::to_string(distance) + ")");
    return result;
}

} // namespace gis_ai
