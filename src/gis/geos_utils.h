#ifndef GIS_AI_GEOS_UTILS_H
#define GIS_AI_GEOS_UTILS_H

#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <geos_c.h>
#include "io/vector_io.h"

namespace gis_ai {
namespace geos_utils {

inline Coordinate CoordFromGeosSeq(GEOSContextHandle_t ctx, const GEOSCoordSequence* seq, unsigned int index) {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    GEOSCoordSeq_getX_r(ctx, seq, index, &x);
    GEOSCoordSeq_getY_r(ctx, seq, index, &y);
    if (GEOSCoordSeq_getZ_r(ctx, seq, index, &z) == 0 || !std::isfinite(z)) {
        z = 0.0;
    }

    return {x, y, z};
}

inline GEOSGeometry* FeatureToGeos(GEOSContextHandle_t ctx, const Feature& feature) {
    GEOSGeometry* geom = nullptr;
    switch (feature.type) {
        case FeatureType::Point: {
            if (!feature.coordinates.empty()) {
                const auto& c = feature.coordinates[0];
                auto* seq = GEOSCoordSeq_create_r(ctx, 1, 3);
                GEOSCoordSeq_setX_r(ctx, seq, 0, c.x);
                GEOSCoordSeq_setY_r(ctx, seq, 0, c.y);
                GEOSCoordSeq_setZ_r(ctx, seq, 0, c.z);
                geom = GEOSGeom_createPoint_r(ctx, seq);
            }
            break;
        }
        case FeatureType::LineString: {
            if (feature.coordinates.size() >= 2) {
                auto* seq = GEOSCoordSeq_create_r(ctx, static_cast<unsigned int>(feature.coordinates.size()), 3);
                for (size_t i = 0; i < feature.coordinates.size(); ++i) {
                    const auto& c = feature.coordinates[i];
                    GEOSCoordSeq_setX_r(ctx, seq, static_cast<unsigned int>(i), c.x);
                    GEOSCoordSeq_setY_r(ctx, seq, static_cast<unsigned int>(i), c.y);
                    GEOSCoordSeq_setZ_r(ctx, seq, static_cast<unsigned int>(i), c.z);
                }
                geom = GEOSGeom_createLineString_r(ctx, seq);
            }
            break;
        }
        case FeatureType::Polygon: {
            if (feature.coordinates.size() >= 4) {
                auto* seq = GEOSCoordSeq_create_r(ctx, static_cast<unsigned int>(feature.coordinates.size()), 3);
                for (size_t i = 0; i < feature.coordinates.size(); ++i) {
                    const auto& c = feature.coordinates[i];
                    GEOSCoordSeq_setX_r(ctx, seq, static_cast<unsigned int>(i), c.x);
                    GEOSCoordSeq_setY_r(ctx, seq, static_cast<unsigned int>(i), c.y);
                    GEOSCoordSeq_setZ_r(ctx, seq, static_cast<unsigned int>(i), c.z);
                }
                auto* ring = GEOSGeom_createLinearRing_r(ctx, seq);
                if (ring) {
                    std::vector<GEOSGeometry*> inner_geoms;
                    inner_geoms.reserve(feature.inner_rings.size());
                    bool valid = true;
                    for (const auto& hole : feature.inner_rings) {
                        if (hole.size() < 4) { valid = false; break; }
                        auto* hole_seq = GEOSCoordSeq_create_r(ctx, static_cast<unsigned int>(hole.size()), 3);
                        for (size_t i = 0; i < hole.size(); ++i) {
                            GEOSCoordSeq_setX_r(ctx, hole_seq, static_cast<unsigned int>(i), hole[i].x);
                            GEOSCoordSeq_setY_r(ctx, hole_seq, static_cast<unsigned int>(i), hole[i].y);
                            GEOSCoordSeq_setZ_r(ctx, hole_seq, static_cast<unsigned int>(i), hole[i].z);
                        }
                        auto* hole_ring = GEOSGeom_createLinearRing_r(ctx, hole_seq);
                        if (!hole_ring) { valid = false; break; }
                        inner_geoms.push_back(hole_ring);
                    }

                    if (valid) {
                        geom = GEOSGeom_createPolygon_r(ctx, ring,
                            inner_geoms.empty() ? nullptr : inner_geoms.data(),
                            static_cast<unsigned int>(inner_geoms.size()));
                    } else {
                        geom = GEOSGeom_createPolygon_r(ctx, ring, nullptr, 0);
                        for (auto* g : inner_geoms) GEOSGeom_destroy_r(ctx, g);
                    }
                }
            }
            break;
        }
    }
    return geom;
}

inline std::unique_ptr<Feature> GeosToFeature(GEOSContextHandle_t ctx, GEOSGeometry* geom, FeatureType type) {
    auto feature = std::make_unique<Feature>();
    feature->type = type;
    if (!geom) return feature;

    switch (type) {
        case FeatureType::Point: {
            auto* seq = GEOSGeom_getCoordSeq_r(ctx, geom);
            if (seq) {
                feature->coordinates.push_back(CoordFromGeosSeq(ctx, seq, 0));
            }
            break;
        }
        case FeatureType::LineString: {
            auto* seq = GEOSGeom_getCoordSeq_r(ctx, geom);
            unsigned int num_points = 0;
            GEOSCoordSeq_getSize_r(ctx, seq, &num_points);
            for (unsigned int i = 0; i < num_points; ++i) {
                feature->coordinates.push_back(CoordFromGeosSeq(ctx, seq, i));
            }
            break;
        }
        case FeatureType::Polygon: {
            auto* ring = GEOSGetExteriorRing_r(ctx, geom);
            if (ring) {
                auto* seq = GEOSGeom_getCoordSeq_r(ctx, ring);
                unsigned int num_points = 0;
                GEOSCoordSeq_getSize_r(ctx, seq, &num_points);
                for (unsigned int i = 0; i < num_points; ++i) {
                    feature->coordinates.push_back(CoordFromGeosSeq(ctx, seq, i));
                }
            }

            int num_holes = GEOSGetNumInteriorRings_r(ctx, geom);
            for (int h = 0; h < num_holes; ++h) {
                auto* hole_ring = GEOSGetInteriorRingN_r(ctx, geom, h);
                if (!hole_ring) continue;
                auto* hole_seq = GEOSGeom_getCoordSeq_r(ctx, hole_ring);
                unsigned int hole_pts = 0;
                GEOSCoordSeq_getSize_r(ctx, hole_seq, &hole_pts);
                std::vector<Coordinate> hole_coords;
                for (unsigned int i = 0; i < hole_pts; ++i) {
                    hole_coords.push_back(CoordFromGeosSeq(ctx, hole_seq, i));
                }
                feature->inner_rings.push_back(std::move(hole_coords));
            }
            break;
        }
    }
    return feature;
}

inline FeatureType GeosTypeToFeatureType(int geos_type) {
    switch (geos_type) {
        case GEOS_POINT:
        case GEOS_MULTIPOINT:
            return FeatureType::Point;
        case GEOS_LINESTRING:
        case GEOS_MULTILINESTRING:
            return FeatureType::LineString;
        case GEOS_POLYGON:
        case GEOS_MULTIPOLYGON:
            return FeatureType::Polygon;
        default:
            return FeatureType::Polygon;
    }
}

inline void ExtractGeosFeatures(GEOSContextHandle_t ctx, GEOSGeometry* geom, FeatureType type,
                                 const std::map<std::string, AttributeValue>& attributes,
                                 std::vector<Feature>& output) {
    if (!geom || GEOSisEmpty_r(ctx, geom) == 1) return;

    int geom_type = GEOSGeomTypeId_r(ctx, geom);

    if (geom_type == GEOS_POLYGON || geom_type == GEOS_LINESTRING || geom_type == GEOS_POINT) {
        auto new_feature = GeosToFeature(ctx, geom, type);
        new_feature->attributes = attributes;
        output.push_back(std::move(*new_feature));
    } else if (geom_type == GEOS_MULTIPOLYGON || geom_type == GEOS_MULTILINESTRING || geom_type == GEOS_MULTIPOINT) {
        int num_geoms = GEOSGetNumGeometries_r(ctx, geom);
        for (int j = 0; j < num_geoms; ++j) {
            const GEOSGeometry* sub = GEOSGetGeometryN_r(ctx, geom, j);
            auto new_feature = GeosToFeature(ctx, const_cast<GEOSGeometry*>(sub), type);
            new_feature->attributes = attributes;
            output.push_back(std::move(*new_feature));
        }
    }
}

class GeosContext {
public:
    GeosContext() : ctx_(GEOS_init_r()) {
        if (ctx_) {
            GEOSContext_setNoticeHandler_r(ctx_, nullptr);
            GEOSContext_setErrorHandler_r(ctx_, nullptr);
        }
    }
    ~GeosContext() {
        if (ctx_) GEOS_finish_r(ctx_);
    }

    GeosContext(const GeosContext&) = delete;
    GeosContext& operator=(const GeosContext&) = delete;

    GEOSContextHandle_t Get() const { return ctx_; }
    operator GEOSContextHandle_t() const { return ctx_; }

private:
    GEOSContextHandle_t ctx_;
};

} // namespace geos_utils
} // namespace gis_ai

#endif // GIS_AI_GEOS_UTILS_H
