#ifndef GIS_AI_BATCH_PROCESSOR_H
#define GIS_AI_BATCH_PROCESSOR_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include "fusion/large_image_seg.h"
#include "core/export.h"

namespace gis_ai {

struct GIS_AI_API BatchResult {
    std::string input_path;
    std::string output_tif_path;
    std::string output_shp_path;
    bool success = false;
    double inference_time_ms = 0.0;
    std::string error_message;
};

class GIS_AI_API BatchProcessor {
public:
    explicit BatchProcessor(const std::string& model_path, int num_threads = 1);
    ~BatchProcessor();

    BatchProcessor(const BatchProcessor&) = delete;
    BatchProcessor& operator=(const BatchProcessor&) = delete;

    std::vector<BatchResult> ProcessDirectory(const std::string& input_dir,
                                               const std::string& output_dir,
                                               bool generate_shp = true);

    std::vector<BatchResult> ProcessFiles(const std::vector<std::string>& input_files,
                                           const std::string& output_dir,
                                           bool generate_shp = true);

    void SetProgressCallback(std::function<void(int, int, const std::string&)> callback);

    void SetSegConfig(const LargeImageSegConfig& config);

    LargeImageSegConfig& SegConfig() { return seg_config_; }

private:
    std::string model_path_;
    int num_threads_;
    LargeImageSegConfig seg_config_;
    std::function<void(int, int, const std::string&)> progress_callback_;
    std::mutex callback_mutex_;
    std::shared_ptr<LargeImageSeg> shared_segmenter_;
    std::mutex segmenter_mutex_;
};

} // namespace gis_ai

#endif // GIS_AI_BATCH_PROCESSOR_H
