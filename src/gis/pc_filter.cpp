#include "gis/pc_filter.h"
#include "core/logger.h"
#include "core/exception.h"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <map>

namespace gis_ai {

std::unique_ptr<PointCloudData> PcFilter::PassThrough(const PointCloudData& input, const PassFilterParams& params) {
    if (input.points.empty()) {
        throw GisAiAlgorithmException("Input point cloud is empty", "PcFilter::PassThrough");
    }

    auto result = std::make_unique<PointCloudData>();
    result->coordinate_system = input.coordinate_system;

    for (const auto& pt : input.points) {
        double val = 0.0;
        if (params.axis == "x" || params.axis == "X") {
            val = pt.x;
        } else if (params.axis == "y" || params.axis == "Y") {
            val = pt.y;
        } else if (params.axis == "z" || params.axis == "Z") {
            val = pt.z;
        } else {
            throw GisAiAlgorithmException("Invalid axis: " + params.axis + " (must be x, y, or z)",
                "PcFilter::PassThrough");
        }

        if (val >= params.min_val && val <= params.max_val) {
            result->points.push_back(pt);
        }
    }

    LOG_INFO("PassThrough filter completed: " + std::to_string(input.points.size()) +
        " -> " + std::to_string(result->points.size()) + " points (axis=" + params.axis +
        ", range=[" + std::to_string(params.min_val) + "," + std::to_string(params.max_val) + "])");
    return result;
}

std::unique_ptr<PointCloudData> PcFilter::StatisticalOutlierRemoval(const PointCloudData& input, int mean_k, double stddev_mul_thresh) {
    if (input.points.empty()) {
        throw GisAiAlgorithmException("Input point cloud is empty", "PcFilter::StatisticalOutlierRemoval");
    }
    if (mean_k <= 0) {
        throw GisAiAlgorithmException("mean_k must be positive", "PcFilter::StatisticalOutlierRemoval");
    }

    size_t n = input.points.size();
    int k = std::min(mean_k, static_cast<int>(n) - 1);

    std::vector<double> distances(n, 0.0);

    for (size_t i = 0; i < n; ++i) {
        std::vector<double> pt_dists;
        pt_dists.reserve(n - 1);

        for (size_t j = 0; j < n; ++j) {
            if (i == j) continue;
            double dx = input.points[i].x - input.points[j].x;
            double dy = input.points[i].y - input.points[j].y;
            double dz = input.points[i].z - input.points[j].z;
            pt_dists.push_back(std::sqrt(dx * dx + dy * dy + dz * dz));
        }

        std::partial_sort(pt_dists.begin(), pt_dists.begin() + k, pt_dists.end());

        double sum = 0.0;
        for (int j = 0; j < k; ++j) {
            sum += pt_dists[j];
        }
        distances[i] = sum / k;
    }

    double mean_dist = 0.0;
    for (double d : distances) mean_dist += d;
    mean_dist /= n;

    double variance = 0.0;
    for (double d : distances) {
        double diff = d - mean_dist;
        variance += diff * diff;
    }
    variance /= n;
    double stddev = std::sqrt(variance);

    double threshold = mean_dist + stddev_mul_thresh * stddev;

    auto result = std::make_unique<PointCloudData>();
    result->coordinate_system = input.coordinate_system;

    for (size_t i = 0; i < n; ++i) {
        if (distances[i] <= threshold) {
            result->points.push_back(input.points[i]);
        }
    }

    LOG_INFO("StatisticalOutlierRemoval completed: " + std::to_string(n) +
        " -> " + std::to_string(result->points.size()) + " points (mean_dist=" +
        std::to_string(mean_dist) + ", threshold=" + std::to_string(threshold) + ")");
    return result;
}

} // namespace gis_ai
