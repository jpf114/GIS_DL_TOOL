#include "fusion/batch_processor.h"
#include "core/logger.h"
#include "core/exception.h"

#include <filesystem>
#include <algorithm>

namespace gis_ai {

BatchProcessor::BatchProcessor(const std::string& model_path, int num_threads)
    : num_threads_(num_threads) {
    raster_seg_ = std::make_unique<RasterSeg>(model_path);
    LOG_INFO("BatchProcessor initialized with " + std::to_string(num_threads) + " threads");
}

BatchProcessor::~BatchProcessor() = default;

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
        if (ext == ".tif" || ext == ".tiff") {
            input_files.push_back(entry.path().string());
        }
    }

    if (input_files.empty()) {
        LOG_WARN("No TIFF files found in " + input_dir);
        return {};
    }

    LOG_INFO("Found " + std::to_string(input_files.size()) + " TIFF files in " + input_dir);
    return ProcessFiles(input_files, output_dir, generate_shp);
}

std::vector<BatchResult> BatchProcessor::ProcessFiles(const std::vector<std::string>& input_files,
                                                        const std::string& output_dir,
                                                        bool generate_shp) {
    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directories(output_dir);
    }

    std::vector<BatchResult> results;
    results.reserve(input_files.size());

    int total = static_cast<int>(input_files.size());
    int completed = 0;

    for (const auto& input_path : input_files) {
        BatchResult result;
        result.input_path = input_path;

        try {
            std::filesystem::path p(input_path);
            std::string stem = p.stem().string();

            result.output_tif_path = (std::filesystem::path(output_dir) / (stem + "_seg.tif")).string();
            if (generate_shp) {
                result.output_shp_path = (std::filesystem::path(output_dir) / (stem + "_seg.shp")).string();
            }

            int ret = raster_seg_->SegmentToFile(input_path, result.output_tif_path,
                generate_shp ? result.output_shp_path : "");

            result.success = (ret == 0);
            result.inference_time_ms = raster_seg_->GetLastInferenceTimeMs();
        } catch (const std::exception& e) {
            result.success = false;
            result.error_message = e.what();
            LOG_ERROR("Batch processing failed for " + input_path + ": " + e.what());
        }

        completed++;
        if (progress_callback_) {
            progress_callback_(completed, total, input_path);
        }

        results.push_back(std::move(result));
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
    progress_callback_ = std::move(callback);
}

} // namespace gis_ai
