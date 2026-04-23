#ifndef GIS_AI_RASTER_NORMALIZE_H
#define GIS_AI_RASTER_NORMALIZE_H

#include <memory>
#include "io/raster_io.h"
#include "core/export.h"

namespace gis_ai {

class GIS_AI_API RasterNormalize {
public:
    RasterNormalize() = default;

    std::unique_ptr<RasterData> Execute(const RasterData& input);

    std::unique_ptr<RasterData> ExecuteMinMax(const RasterData& input, float min_val, float max_val);
};

} // namespace gis_ai

#endif // GIS_AI_RASTER_NORMALIZE_H
