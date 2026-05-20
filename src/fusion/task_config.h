#ifndef GIS_AI_TASK_CONFIG_H
#define GIS_AI_TASK_CONFIG_H

#include <string>
#include <vector>
#include "fusion/large_image_seg.h"
#include "core/export.h"

namespace gis_ai {

enum class GIS_AI_API TaskType {
    Segment,
    SegmentToPolygon,
    BatchSegment,
    BatchInference,
    Inference,
    Preprocess,
    VectorSimplify,
    VectorBuffer,
    RasterMosaic,
    RasterResample
};

struct GIS_AI_API TaskConfig {
    TaskType task_type = TaskType::Segment;

    std::string model_path;

    std::string input_path;
    std::string output_tif_path;
    std::string output_shp_path;

    std::string input_dir;
    std::string output_dir;

    std::string output_path;

    LargeImageSegConfig seg_config;

    double resample_resolution = 0.0;
    std::string resample_method = "nearest";

    double simplify_tolerance = 1.0;

    double buffer_distance = 0.0;

    int num_threads = 1;
    bool verbose = false;
    std::string log_file = "gis_ai.log";

    static TaskConfig LoadFromFile(const std::string& path);
    static TaskConfig LoadFromString(const std::string& json_str);
    void SaveToFile(const std::string& path) const;
    std::string ToString() const;
};

struct GIS_AI_API TaskReport {
    bool success = false;
    std::string error_message;
    double total_time_ms = 0.0;
    SegmentationStats seg_stats;
    std::vector<std::string> output_files;

    std::string task_type_name;
    std::string input_path;
    std::string start_time;
    std::string end_time;
    int error_code = 0;

    std::string ToString() const;
    std::string ToJson() const;
    void SaveReport(const std::string& path) const;
};

class GIS_AI_API TaskRunner {
public:
    static TaskReport Execute(const TaskConfig& config);
};

} // namespace gis_ai

#endif // GIS_AI_TASK_CONFIG_H
