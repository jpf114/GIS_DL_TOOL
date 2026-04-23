#ifndef GIS_AI_RASTER_CLIP_H
#define GIS_AI_RASTER_CLIP_H

#include <memory>
#include <vector>
#include "io/raster_io.h"
#include "core/export.h"

namespace gis_ai {

struct GIS_AI_API BoundingBox {
    double min_x = 0.0;
    double max_x = 0.0;
    double min_y = 0.0;
    double max_y = 0.0;
};

class GIS_AI_API RasterClip {
public:
    RasterClip() = default;

    std::unique_ptr<RasterData> Execute(const RasterData& input, const BoundingBox& bbox);

    std::unique_ptr<RasterData> ExecuteByPixel(const RasterData& input, int x_off, int y_off, int width, int height);
};

} // namespace gis_ai

#endif // GIS_AI_RASTER_CLIP_H
