#include "fusion/task_config.h"
#include "fusion/large_image_seg.h"
#include "fusion/batch_processor.h"
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "core/logger.h"
#include "core/exception.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <chrono>

namespace gis_ai {

TaskConfig TaskConfig::LoadFromFile(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        throw GisAiConfigException("Task config file not found: " + path);
    }
    std::ifstream file(path);
    if (!file.is_open()) {
        throw GisAiConfigException("Cannot open task config file: " + path);
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return LoadFromString(content);
}

TaskConfig TaskConfig::LoadFromString(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    TaskConfig config;

    std::string type_str = j.value("task_type", "segment");
    if (type_str == "segment") {
        config.task_type = TaskType::Segment;
    } else if (type_str == "segment_to_polygon") {
        config.task_type = TaskType::SegmentToPolygon;
    } else if (type_str == "batch_segment") {
        config.task_type = TaskType::BatchSegment;
    } else if (type_str == "inference") {
        config.task_type = TaskType::Inference;
    } else if (type_str == "preprocess") {
        config.task_type = TaskType::Preprocess;
    } else if (type_str == "vector_simplify") {
        config.task_type = TaskType::VectorSimplify;
    } else if (type_str == "vector_buffer") {
        config.task_type = TaskType::VectorBuffer;
    } else if (type_str == "raster_mosaic") {
        config.task_type = TaskType::RasterMosaic;
    } else if (type_str == "raster_resample") {
        config.task_type = TaskType::RasterResample;
    }

    config.model_path = j.value("model_path", "");
    config.input_path = j.value("input_path", "");
    config.output_tif_path = j.value("output_tif_path", "");
    config.output_shp_path = j.value("output_shp_path", "");
    config.input_dir = j.value("input_dir", "");
    config.output_dir = j.value("output_dir", "");
    config.output_path = j.value("output_path", "");
    config.num_threads = j.value("num_threads", 1);
    config.verbose = j.value("verbose", false);
    config.log_file = j.value("log_file", "gis_ai.log");

    config.resample_resolution = j.value("resample_resolution", 0.0);
    config.resample_method = j.value("resample_method", "nearest");
    config.simplify_tolerance = j.value("simplify_tolerance", 1.0);
    config.buffer_distance = j.value("buffer_distance", 0.0);

    if (j.contains("segment")) {
        auto& s = j["segment"];
        config.seg_config.tile_size = s.value("tile_size", 512);
        config.seg_config.stride = s.value("stride", 384);
        config.seg_config.target_channels = s.value("target_channels", 3);
        config.seg_config.input_is_uint8 = s.value("input_is_uint8", true);
        config.seg_config.mask_threshold = s.value("mask_threshold", 0.5f);
        config.seg_config.target_class = s.value("target_class", 1);
        config.seg_config.skip_nodata = s.value("skip_nodata", true);
        config.seg_config.nodata_value = s.value("nodata_value", 0.0f);
        config.seg_config.min_polygon_area = s.value("min_polygon_area", 100.0);
        config.seg_config.simplify_tolerance = s.value("simplify_tolerance", 1.0);
        config.seg_config.fix_topology = s.value("fix_topology", true);
        config.seg_config.gaussian_sigma = s.value("gaussian_sigma", 0.5f);

        std::string blend_str = s.value("blend_mode", "gaussian");
        if (blend_str == "none") {
            config.seg_config.blend_mode = BlendMode::None;
        } else if (blend_str == "linear") {
            config.seg_config.blend_mode = BlendMode::Linear;
        } else {
            config.seg_config.blend_mode = BlendMode::Gaussian;
        }

        std::string norm_str = s.value("normalize_mode", "imagenet");
        if (norm_str == "none") {
            config.seg_config.normalize_mode = NormalizeMode::None;
        } else if (norm_str == "minmax") {
            config.seg_config.normalize_mode = NormalizeMode::MinMax01;
        } else if (norm_str == "zscore") {
            config.seg_config.normalize_mode = NormalizeMode::ZScore;
        } else if (norm_str == "custom") {
            config.seg_config.normalize_mode = NormalizeMode::Custom;
        } else {
            config.seg_config.normalize_mode = NormalizeMode::ImageNet;
        }

        if (s.contains("mean")) {
            auto& m = s["mean"];
            if (m.is_array() && m.size() >= 3) {
                config.seg_config.mean_r = m[0].get<float>();
                config.seg_config.mean_g = m[1].get<float>();
                config.seg_config.mean_b = m[2].get<float>();
            }
        }
        if (s.contains("std")) {
            auto& st = s["std"];
            if (st.is_array() && st.size() >= 3) {
                config.seg_config.std_r = st[0].get<float>();
                config.seg_config.std_g = st[1].get<float>();
                config.seg_config.std_b = st[2].get<float>();
            }
        }
    }

    return config;
}

void TaskConfig::SaveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw GisAiConfigException("Cannot open file for writing: " + path);
    }
    file << ToString() << "\n";
}

std::string TaskConfig::ToString() const {
    nlohmann::json j;

    switch (task_type) {
        case TaskType::Segment: j["task_type"] = "segment"; break;
        case TaskType::SegmentToPolygon: j["task_type"] = "segment_to_polygon"; break;
        case TaskType::BatchSegment: j["task_type"] = "batch_segment"; break;
        case TaskType::Inference: j["task_type"] = "inference"; break;
        case TaskType::Preprocess: j["task_type"] = "preprocess"; break;
        case TaskType::VectorSimplify: j["task_type"] = "vector_simplify"; break;
        case TaskType::VectorBuffer: j["task_type"] = "vector_buffer"; break;
        case TaskType::RasterMosaic: j["task_type"] = "raster_mosaic"; break;
        case TaskType::RasterResample: j["task_type"] = "raster_resample"; break;
    }

    j["model_path"] = model_path;
    j["input_path"] = input_path;
    j["output_tif_path"] = output_tif_path;
    j["output_shp_path"] = output_shp_path;
    j["input_dir"] = input_dir;
    j["output_dir"] = output_dir;
    j["output_path"] = output_path;
    j["num_threads"] = num_threads;
    j["verbose"] = verbose;
    j["log_file"] = log_file;

    j["resample_resolution"] = resample_resolution;
    j["resample_method"] = resample_method;
    j["simplify_tolerance"] = simplify_tolerance;
    j["buffer_distance"] = buffer_distance;

    auto& s = j["segment"];
    s["tile_size"] = seg_config.tile_size;
    s["stride"] = seg_config.stride;
    s["target_channels"] = seg_config.target_channels;
    s["input_is_uint8"] = seg_config.input_is_uint8;
    s["mask_threshold"] = seg_config.mask_threshold;
    s["target_class"] = seg_config.target_class;
    s["skip_nodata"] = seg_config.skip_nodata;
    s["nodata_value"] = seg_config.nodata_value;
    s["min_polygon_area"] = seg_config.min_polygon_area;
    s["simplify_tolerance"] = seg_config.simplify_tolerance;
    s["fix_topology"] = seg_config.fix_topology;
    s["gaussian_sigma"] = seg_config.gaussian_sigma;

    switch (seg_config.blend_mode) {
        case BlendMode::None: s["blend_mode"] = "none"; break;
        case BlendMode::Linear: s["blend_mode"] = "linear"; break;
        case BlendMode::Gaussian: s["blend_mode"] = "gaussian"; break;
    }

    switch (seg_config.normalize_mode) {
        case NormalizeMode::None: s["normalize_mode"] = "none"; break;
        case NormalizeMode::ImageNet: s["normalize_mode"] = "imagenet"; break;
        case NormalizeMode::MinMax01: s["normalize_mode"] = "minmax"; break;
        case NormalizeMode::ZScore: s["normalize_mode"] = "zscore"; break;
        case NormalizeMode::Custom: s["normalize_mode"] = "custom"; break;
    }

    s["mean"] = {seg_config.mean_r, seg_config.mean_g, seg_config.mean_b};
    s["std"] = {seg_config.std_r, seg_config.std_g, seg_config.std_b};

    return j.dump(2);
}

std::string TaskReport::ToString() const {
    nlohmann::json j;
    j["success"] = success;
    if (!error_message.empty()) j["error_message"] = error_message;
    j["total_time_ms"] = total_time_ms;
    j["output_files"] = output_files;

    auto& stats = j["stats"];
    stats["total_tiles"] = seg_stats.total_tiles;
    stats["skipped_tiles"] = seg_stats.skipped_tiles;
    stats["inferred_tiles"] = seg_stats.inferred_tiles;
    stats["total_inference_time_ms"] = seg_stats.total_inference_time_ms;
    stats["polygon_count"] = seg_stats.polygon_count;
    stats["total_polygon_area"] = seg_stats.total_polygon_area;

    return j.dump(2);
}

TaskReport TaskRunner::Execute(const TaskConfig& config) {
    TaskReport report;
    auto start = std::chrono::high_resolution_clock::now();

    try {
        if (config.model_path.empty()) {
            throw GisAiConfigException("model_path is required");
        }

        switch (config.task_type) {
            case TaskType::Segment:
            case TaskType::SegmentToPolygon: {
                if (config.input_path.empty()) {
                    throw GisAiConfigException("input_path is required for segment task");
                }

                LargeImageSeg seg(config.model_path);

                std::string output_tif = config.output_tif_path;
                if (output_tif.empty()) {
                    auto stem = std::filesystem::path(config.input_path).stem().string();
                    auto parent = std::filesystem::path(config.input_path).parent_path();
                    output_tif = (parent / (stem + "_seg.tif")).string();
                }

                std::string output_shp = config.output_shp_path;
                if (config.task_type == TaskType::SegmentToPolygon && output_shp.empty()) {
                    auto stem = std::filesystem::path(config.input_path).stem().string();
                    auto parent = std::filesystem::path(config.input_path).parent_path();
                    output_shp = (parent / (stem + "_seg.shp")).string();
                }

                int ret = seg.SegmentToFile(config.input_path, output_tif, output_shp, config.seg_config);
                report.success = (ret == 0);
                report.seg_stats = seg.GetLastStats();
                report.output_files.push_back(output_tif);
                if (!output_shp.empty()) {
                    report.output_files.push_back(output_shp);
                }
                break;
            }
            case TaskType::BatchSegment: {
                if (config.input_dir.empty()) {
                    throw GisAiConfigException("input_dir is required for batch_segment task");
                }

                BatchProcessor processor(config.model_path, config.num_threads);
                auto results = processor.ProcessDirectory(
                    config.input_dir, config.output_dir, true);

                report.success = true;
                for (const auto& r : results) {
                    if (r.success) {
                        report.output_files.push_back(r.output_tif_path);
                        if (!r.output_shp_path.empty()) {
                            report.output_files.push_back(r.output_shp_path);
                        }
                    } else {
                        report.success = false;
                        if (!report.error_message.empty()) report.error_message += "; ";
                        report.error_message += r.input_path + ": " + r.error_message;
                    }
                }
                break;
            }
        }
    } catch (const std::exception& e) {
        report.success = false;
        report.error_message = e.what();
        LOG_ERROR("Task execution failed: " + std::string(e.what()));
    }

    auto end = std::chrono::high_resolution_clock::now();
    report.total_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    return report;
}

} // namespace gis_ai
