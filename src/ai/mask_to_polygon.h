#ifndef GIS_AI_MASK_TO_POLYGON_H
#define GIS_AI_MASK_TO_POLYGON_H

#include <memory>
#include <vector>
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "core/export.h"

namespace gis_ai {

struct GIS_AI_API ContourPoint {
    double x;
    double y;
};

class GIS_AI_API MaskToPolygon {
public:
    MaskToPolygon() = default;

    std::unique_ptr<VectorData> Execute(const std::vector<uint8_t>& mask, int width, int height,
                                         const double* geotransform, uint8_t target_class = 1);

    std::unique_ptr<VectorData> ExecuteFromRaster(const RasterData& raster, uint8_t target_class = 1);
};

} // namespace gis_ai

#endif // GIS_AI_MASK_TO_POLYGON_H
