#include "gis/pc_downsample.h"
#include "core/logger.h"
#include "core/exception.h"

#include <cmath>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <numeric>

namespace gis_ai {

struct VoxelKey {
    int64_t ix, iy, iz;

    bool operator==(const VoxelKey& other) const {
        return ix == other.ix && iy == other.iy && iz == other.iz;
    }
};

struct VoxelKeyHash {
    size_t operator()(const VoxelKey& k) const {
        size_t h = static_cast<size_t>(k.ix);
        h ^= static_cast<size_t>(k.iy) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= static_cast<size_t>(k.iz) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

std::unique_ptr<PointCloudData> PcDownsample::VoxelGrid(const PointCloudData& input, double voxel_size) {
    if (input.points.empty()) {
        throw GisAiAlgorithmException("Input point cloud is empty", "PcDownsample::VoxelGrid");
    }
    if (voxel_size <= 0.0) {
        throw GisAiAlgorithmException("Voxel size must be positive", "PcDownsample::VoxelGrid");
    }

    std::unordered_map<VoxelKey, std::vector<size_t>, VoxelKeyHash> voxel_map;

    for (size_t i = 0; i < input.points.size(); ++i) {
        const auto& pt = input.points[i];
        VoxelKey key;
        key.ix = static_cast<int64_t>(std::floor(pt.x / voxel_size));
        key.iy = static_cast<int64_t>(std::floor(pt.y / voxel_size));
        key.iz = static_cast<int64_t>(std::floor(pt.z / voxel_size));
        voxel_map[key].push_back(i);
    }

    auto result = std::make_unique<PointCloudData>();
    result->coordinate_system = input.coordinate_system;

    for (const auto& [key, indices] : voxel_map) {
        double sum_x = 0.0, sum_y = 0.0, sum_z = 0.0;
        float sum_intensity = 0.0f;
        uint8_t last_class = 0;

        for (size_t idx : indices) {
            const auto& pt = input.points[idx];
            sum_x += pt.x;
            sum_y += pt.y;
            sum_z += pt.z;
            sum_intensity += pt.intensity;
            last_class = pt.classification;
        }

        double count = static_cast<double>(indices.size());
        Point centroid;
        centroid.x = sum_x / count;
        centroid.y = sum_y / count;
        centroid.z = sum_z / count;
        centroid.intensity = sum_intensity / static_cast<float>(count);
        centroid.classification = last_class;

        result->points.push_back(centroid);
    }

    LOG_INFO("VoxelGrid downsample completed: " + std::to_string(input.points.size()) +
        " -> " + std::to_string(result->points.size()) + " points (voxel_size=" +
        std::to_string(voxel_size) + ")");
    return result;
}

std::unique_ptr<PointCloudData> PcDownsample::RandomDownsample(const PointCloudData& input, double ratio) {
    if (input.points.empty()) {
        throw GisAiAlgorithmException("Input point cloud is empty", "PcDownsample::RandomDownsample");
    }
    if (ratio <= 0.0 || ratio > 1.0) {
        throw GisAiAlgorithmException("Ratio must be in (0, 1]", "PcDownsample::RandomDownsample");
    }

    size_t target_count = static_cast<size_t>(std::ceil(input.points.size() * ratio));
    target_count = std::max(size_t(1), std::min(target_count, input.points.size()));

    std::vector<size_t> indices(input.points.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(indices.begin(), indices.end(), gen);

    indices.resize(target_count);
    std::sort(indices.begin(), indices.end());

    auto result = std::make_unique<PointCloudData>();
    result->coordinate_system = input.coordinate_system;

    for (size_t idx : indices) {
        result->points.push_back(input.points[idx]);
    }

    LOG_INFO("RandomDownsample completed: " + std::to_string(input.points.size()) +
        " -> " + std::to_string(result->points.size()) + " points (ratio=" +
        std::to_string(ratio) + ")");
    return result;
}

} // namespace gis_ai
