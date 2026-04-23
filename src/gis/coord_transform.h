#ifndef GIS_AI_COORD_TRANSFORM_H
#define GIS_AI_COORD_TRANSFORM_H

#include <memory>
#include <string>
#include <vector>
#include "io/vector_io.h"
#include "io/raster_io.h"
#include "io/pointcloud_io.h"
#include "core/export.h"

namespace gis_ai {

struct GIS_AI_API CoordPair {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

class GIS_AI_API CoordTransform {
public:
    CoordTransform() = default;

    CoordPair Transform(double x, double y, double z, const std::string& src_crs, const std::string& dst_crs);

    std::vector<CoordPair> TransformBatch(const std::vector<CoordPair>& coords,
                                           const std::string& src_crs, const std::string& dst_crs);

    std::unique_ptr<VectorData> TransformVector(const VectorData& input, const std::string& dst_crs);

    std::unique_ptr<PointCloudData> TransformPointCloud(const PointCloudData& input, const std::string& dst_crs);
};

} // namespace gis_ai

#endif // GIS_AI_COORD_TRANSFORM_H
