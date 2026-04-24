#ifndef GIS_AI_BATCH_PROCESSOR_H
#define GIS_AI_BATCH_PROCESSOR_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "io/raster_io.h"
#include "fusion/raster_seg.h"
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

private:
    std::string model_path_;
    std::unique_ptr<RasterSeg> raster_seg_;
    int num_threads_;
    std::function<void(int, int, const std::string&)> progress_callback_;
};

} // namespace gis_ai

#endif // GIS_AI_BATCH_PROCESSOR_H
