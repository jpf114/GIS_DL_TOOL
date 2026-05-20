#include "fusion/batch_processor.h"
#include "core/logger.h"
#include "core/exception.h"

#include <filesystem>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>

namespace gis_ai {

BatchProcessor::BatchProcessor(const std::string& model_path, int num_threads)
    : model_path_(model_path),
      num_threads_(std::max(1, num_threads)) {
    LOG_INFO("BatchProcessor initialized with " + std::to_string(num_threads_) + " threads");
}

BatchProcessor::~BatchProcessor() = default;

void BatchProcessor::SetSegConfig(const LargeImageSegConfig& config) {
    seg_config_ = config;
}

std::vector<BatchResult> BatchProcessor::ProcessDirectory(const std::string& input_dir,
                                                            const std::string& output_dir,
                                                            bool generate_shp) {
    std::vector<std::string> input_files;

    if (!std::filesystem::exists(input_dir)) {
        throw GisAiAlgorithmException("Input directory not found: " + input_dir,
            "BatchProcessor::ProcessDirectory");
    }

    for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".tif" || ext == ".tiff" || ext == ".img" || ext == ".png") {
            input_files.push_back(entry.path().string());
        }
    }

    if (input_files.empty()) {
        LOG_WARN("No raster files found in " + input_dir);
        return {};
    }

    LOG_INFO("Found " + std::to_string(input_files.size()) + " raster files in " + input_dir);
    return ProcessFiles(input_files, output_dir, generate_shp);
}

std::vector<BatchResult> BatchProcessor::ProcessFiles(const std::vector<std::string>& input_files,
                                                        const std::string& output_dir,
                                                        bool generate_shp) {
    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directories(output_dir);
    }

    std::vector<BatchResult> results(input_files.size());

    int total = static_cast<int>(input_files.size());
    std::atomic<size_t> next_index{0};
    std::atomic<int> completed{0};

    shared_segmenter_ = std::make_shared<LargeImageSeg>(model_path_);

    auto process_one = [&](LargeImageSeg& segmenter, size_t index) {
        const auto& input_path = input_files[index];
        BatchResult result;
        result.input_path = input_path;

        try {
            std::filesystem::path p(input_path);
            std::string stem = p.stem().string();

            result.output_tif_path = (std::filesystem::path(output_dir) / (stem + "_seg.tif")).string();
            if (generate_shp) {
                result.output_shp_path = (std::filesystem::path(output_dir) / (stem + "_seg.shp")).string();
            }

            int ret = segmenter.SegmentToFile(input_path, result.output_tif_path,
                generate_shp ? result.output_shp_path : "", seg_config_);

            result.success = (ret == 0);
            result.inference_time_ms = segmenter.GetLastStats().total_inference_time_ms;
        } catch (const std::exception& e) {
            result.success = false;
            result.error_message = e.what();
            LOG_ERROR("Batch processing failed for " + input_path + ": " + e.what());
        }

        results[index] = std::move(result);
        int finished = completed.fetch_add(1) + 1;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            if (progress_callback_) {
                progress_callback_(finished, total, input_path);
            }
        }
    };

    const int worker_count = std::min(num_threads_, total);
    if (worker_count == 1) {
        for (size_t i = 0; i < input_files.size(); ++i) {
            process_one(*shared_segmenter_, i);
        }
    } else {
        std::vector<std::thread> workers;
        workers.reserve(worker_count);

        for (int worker = 0; worker < worker_count; ++worker) {
            workers.emplace_back([&, worker]() {
                while (true) {
                    size_t index = next_index.fetch_add(1);
                    if (index >= input_files.size()) {
                        break;
                    }
                    std::lock_guard<std::mutex> lock(segmenter_mutex_);
                    process_one(*shared_segmenter_, index);
                }
            });
        }

        for (auto& worker : workers) {
            worker.join();
        }
    }

    int success_count = 0;
    for (const auto& r : results) {
        if (r.success) success_count++;
    }

    LOG_INFO("Batch processing completed: " + std::to_string(success_count) + "/" +
        std::to_string(total) + " succeeded");
    return results;
}

void BatchProcessor::SetProgressCallback(std::function<void(int, int, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    progress_callback_ = std::move(callback);
}

} // namespace gis_ai
