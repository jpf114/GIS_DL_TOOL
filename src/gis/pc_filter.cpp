#include "gis/pc_filter.h"
#include "core/logger.h"
#include "core/exception.h"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <vector>

namespace gis_ai {

class KdTree {
public:
    struct Node {
        int index = -1;
        int axis = 0;
        int left = -1;
        int right = -1;
    };

    void Build(const std::vector<Point>& points) {
        if (points.empty()) return;
        indices_.resize(points.size());
        std::iota(indices_.begin(), indices_.end(), 0);
        points_ = &points;
        nodes_.clear();
        nodes_.reserve(points.size());
        BuildSubtree(0, static_cast<int>(points.size()), 0);
        points_ = nullptr;
    }

    void KnnSearch(const std::vector<Point>& points, int query_idx, int k,
                   std::vector<int>& result_indices) const {
        if (nodes_.empty() || k <= 0) return;
        result_indices.clear();
        if (k >= static_cast<int>(points.size())) {
            result_indices.resize(points.size() - 1);
            int idx = 0;
            for (int i = 0; i < static_cast<int>(points.size()); ++i) {
                if (i != query_idx) result_indices[idx++] = i;
            }
            return;
        }

        const auto& qp = points[query_idx];
        std::vector<std::pair<double, int>> heap;
        heap.reserve(k + 1);
        double best_dist = std::numeric_limits<double>::max();

        SearchNode(points, 0, qp, k, heap, best_dist);

        result_indices.resize(heap.size());
        for (size_t i = 0; i < heap.size(); ++i) {
            result_indices[i] = heap[i].second;
        }
    }

private:
    int BuildSubtree(int start, int end, int depth) {
        if (start >= end) return -1;

        int axis = depth % 3;
        int mid = (start + end) / 2;

        auto cmp = [this, axis](int a, int b) {
            const auto& pa = (*points_)[a];
            const auto& pb = (*points_)[b];
            if (axis == 0) return pa.x < pb.x;
            if (axis == 1) return pa.y < pb.y;
            return pa.z < pb.z;
        };
        std::nth_element(indices_.begin() + start, indices_.begin() + mid, indices_.begin() + end, cmp);

        int node_idx = static_cast<int>(nodes_.size());
        nodes_.push_back({indices_[mid], axis, -1, -1});

        int left = BuildSubtree(start, mid, depth + 1);
        int right = BuildSubtree(mid + 1, end, depth + 1);

        nodes_[node_idx].left = left;
        nodes_[node_idx].right = right;

        return node_idx;
    }

    void SearchNode(const std::vector<Point>& points, int node_idx,
                    const Point& qp, int k,
                    std::vector<std::pair<double, int>>& heap,
                    double& best_dist) const {
        if (node_idx < 0 || node_idx >= static_cast<int>(nodes_.size())) return;

        const auto& node = nodes_[node_idx];
        const auto& np = points[node.index];

        double dx = qp.x - np.x;
        double dy = qp.y - np.y;
        double dz = qp.z - np.z;
        double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (heap.size() < static_cast<size_t>(k)) {
            heap.push_back({dist, node.index});
            std::push_heap(heap.begin(), heap.end());
            if (heap.size() == static_cast<size_t>(k)) {
                best_dist = heap[0].first;
            }
        } else if (dist < best_dist) {
            std::pop_heap(heap.begin(), heap.end());
            heap.back() = {dist, node.index};
            std::push_heap(heap.begin(), heap.end());
            best_dist = heap[0].first;
        }

        double plane_diff = 0.0;
        if (node.axis == 0) plane_diff = qp.x - np.x;
        else if (node.axis == 1) plane_diff = qp.y - np.y;
        else plane_diff = qp.z - np.z;

        int first = plane_diff < 0 ? node.left : node.right;
        int second = plane_diff < 0 ? node.right : node.left;

        SearchNode(points, first, qp, k, heap, best_dist);

        if (heap.size() < static_cast<size_t>(k) || std::abs(plane_diff) < best_dist) {
            SearchNode(points, second, qp, k, heap, best_dist);
        }
    }

    std::vector<int> indices_;
    std::vector<Node> nodes_;
    const std::vector<Point>* points_ = nullptr;
};

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

    KdTree tree;
    tree.Build(input.points);

    std::vector<double> distances(n, 0.0);
    std::vector<int> neighbors;

    for (size_t i = 0; i < n; ++i) {
        tree.KnnSearch(input.points, static_cast<int>(i), k, neighbors);

        double sum = 0.0;
        int count = std::min(k, static_cast<int>(neighbors.size()));
        for (int j = 0; j < count; ++j) {
            const auto& nb = input.points[neighbors[j]];
            double dx = input.points[i].x - nb.x;
            double dy = input.points[i].y - nb.y;
            double dz = input.points[i].z - nb.z;
            sum += std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        distances[i] = count > 0 ? sum / count : 0.0;
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
