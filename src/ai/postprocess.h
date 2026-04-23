#ifndef GIS_AI_POSTPROCESS_H
#define GIS_AI_POSTPROCESS_H

#include <vector>
#include "io/raster_io.h"
#include "core/export.h"

namespace gis_ai {

class GIS_AI_API Postprocess {
public:
    Postprocess() = default;

    std::vector<uint8_t> SigmoidArgmax(const std::vector<float>& output_data,
                                        int64_t height, int64_t width, int64_t num_classes);

    std::vector<float> Sigmoid(const std::vector<float>& logits);

    std::vector<uint8_t> ThresholdMask(const std::vector<float>& mask, float threshold = 0.5f);

    RasterData MaskToRaster(const std::vector<uint8_t>& mask, int width, int height,
                             const double* geotransform, const std::string& projection);
};

} // namespace gis_ai

#endif // GIS_AI_POSTPROCESS_H
