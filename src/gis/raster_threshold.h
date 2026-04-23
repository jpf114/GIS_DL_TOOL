#ifndef GIS_AI_RASTER_THRESHOLD_H
#define GIS_AI_RASTER_THRESHOLD_H

#include <memory>
#include "io/raster_io.h"
#include "core/export.h"

namespace gis_ai {

class GIS_AI_API RasterThreshold {
public:
    RasterThreshold() = default;

    std::unique_ptr<RasterData> Execute(const RasterData& input, double threshold, int band_index = 0);

    std::unique_ptr<RasterData> ExecuteRange(const RasterData& input, double min_val, double max_val, int band_index = 0);
};

} // namespace gis_ai

#endif // GIS_AI_RASTER_THRESHOLD_H
