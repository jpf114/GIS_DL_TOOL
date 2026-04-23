#ifndef GIS_AI_PC_DOWNSAMPLE_H
#define GIS_AI_PC_DOWNSAMPLE_H

#include <memory>
#include "io/pointcloud_io.h"
#include "core/export.h"

namespace gis_ai {

class GIS_AI_API PcDownsample {
public:
    PcDownsample() = default;

    std::unique_ptr<PointCloudData> VoxelGrid(const PointCloudData& input, double voxel_size);

    std::unique_ptr<PointCloudData> RandomDownsample(const PointCloudData& input, double ratio);
};

} // namespace gis_ai

#endif // GIS_AI_PC_DOWNSAMPLE_H
