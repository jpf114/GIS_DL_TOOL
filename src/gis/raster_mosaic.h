#ifndef GIS_AI_RASTER_MOSAIC_H
#define GIS_AI_RASTER_MOSAIC_H

#include <memory>
#include <vector>
#include "io/raster_io.h"
#include "core/export.h"

namespace gis_ai {

class GIS_AI_API RasterMosaic {
public:
    RasterMosaic() = default;

    std::unique_ptr<RasterData> Execute(const std::vector<std::reference_wrapper<const RasterData>>& rasters);
};

} // namespace gis_ai

#endif // GIS_AI_RASTER_MOSAIC_H
