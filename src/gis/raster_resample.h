#ifndef GIS_AI_RASTER_RESAMPLE_H
#define GIS_AI_RASTER_RESAMPLE_H

#include <memory>
#include <string>
#include "io/raster_io.h"
#include "core/export.h"

namespace gis_ai {

enum class GIS_AI_API ResampleMethod {
    Nearest,
    Bilinear
};

class GIS_AI_API RasterResample {
public:
    RasterResample() = default;

    std::unique_ptr<RasterData> Execute(const RasterData& input, int new_width, int new_height,
                                         ResampleMethod method = ResampleMethod::Nearest);
};

} // namespace gis_ai

#endif // GIS_AI_RASTER_RESAMPLE_H
