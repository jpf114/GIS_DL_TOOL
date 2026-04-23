#ifndef GIS_AI_PC_FILTER_H
#define GIS_AI_PC_FILTER_H

#include <memory>
#include <string>
#include "io/pointcloud_io.h"
#include "core/export.h"

namespace gis_ai {

struct GIS_AI_API PassFilterParams {
    std::string axis = "z";
    double min_val = 0.0;
    double max_val = 100.0;
};

struct GIS_AI_API GaussianFilterParams {
    double sigma = 1.0;
    int kernel_size = 3;
};

class GIS_AI_API PcFilter {
public:
    PcFilter() = default;

    std::unique_ptr<PointCloudData> PassThrough(const PointCloudData& input, const PassFilterParams& params);

    std::unique_ptr<PointCloudData> StatisticalOutlierRemoval(const PointCloudData& input, int mean_k = 50, double stddev_mul_thresh = 1.0);
};

} // namespace gis_ai

#endif // GIS_AI_PC_FILTER_H
