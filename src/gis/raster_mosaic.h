#ifndef GIS_AI_RASTER_MOSAIC_H
#define GIS_AI_RASTER_MOSAIC_H

#include <memory>
#include <vector>
#include <string>
#include "io/raster_io.h"
#include "core/export.h"

namespace gis_ai {

enum class GIS_AI_API MosaicStrategy {
    Overwrite,
    First,
    Mean,
    Max,
    Min
};

struct GIS_AI_API MosaicConfig {
    MosaicStrategy strategy = MosaicStrategy::Overwrite;
    float nodata_value = std::numeric_limits<float>::quiet_NaN();
};

class GIS_AI_API RasterMosaic {
public:
    RasterMosaic() = default;

    std::unique_ptr<RasterData> Execute(const std::vector<std::reference_wrapper<const RasterData>>& rasters,
                                         const MosaicConfig& config = MosaicConfig());
};

} // namespace gis_ai

#endif // GIS_AI_RASTER_MOSAIC_H
