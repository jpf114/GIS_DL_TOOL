#include "action_dispatcher.h"
#include "qt_progress_reporter.h"
#include "param_utils.h"

#include "fusion/large_image_seg.h"
#include "io/raster_io.h"
#include "io/vector_io.h"
#include "gis/raster_resample.h"
#include "gis/raster_normalize.h"
#include "gis/raster_clip.h"
#include "gis/raster_mosaic.h"
#include "gis/raster_threshold.h"
#include "gis/vector_simplify.h"
#include "gis/vector_buffer.h"
#include "gis/vector_clip.h"
#include "core/logger.h"

#include <filesystem>
#include <array>
#include <sstream>

namespace gis_ai::gui {

namespace {

BlendMode parseBlendMode(const std::string& s) {
    if (s.find("Linear") != std::string::npos) return BlendMode::Linear;
    if (s.find("Gaussian") != std::string::npos) return BlendMode::Gaussian;
    return BlendMode::None;
}

LargeImageSegConfig buildSegConfig(const std::map<std::string, ParamValue>& params) {
    LargeImageSegConfig config;
    config.tile_size = getIntParam(params, "tile_size", 512);
    config.stride = getIntParam(params, "stride", 256);
    config.target_class = static_cast<uint8_t>(getIntParam(params, "target_class", 1));
    config.skip_nodata = getBoolParam(params, "skip_nodata", true);
    config.blend_mode = parseBlendMode(getStringParam(params, "blend_mode"));
    return config;
}

LargeImageSegConfig buildInferenceConfig(const std::map<std::string, ParamValue>& params) {
    LargeImageSegConfig config;
    config.target_class = static_cast<uint8_t>(getIntParam(params, "target_class", 1));
    return config;
}

}

bool executeAction(const std::string& pluginName, const std::string& actionKey,
                   const std::map<std::string, ParamValue>& params,
                   QtProgressReporter& reporter) {
    reporter.onProgress(0.0);
    reporter.onMessage("开始执行: " + pluginName + "/" + actionKey);

    try {
        if (pluginName == "segment") {
            auto model_path = getStringParam(params, "model_path");
            auto input_raster = getStringParam(params, "input_raster");
            auto output_tif = getStringParam(params, "output_tif");
            auto output_shp = getStringParam(params, "output_shp");
            auto config = buildSegConfig(params);

            LargeImageSeg seg(model_path);
            seg.SetProgressCallback([&reporter](int current, int total, const std::string& msg) {
                if (total > 0) {
                    reporter.onProgress(static_cast<double>(current) / total);
                }
                reporter.onMessage(msg);
            });

            if (actionKey == "segment_full") {
                reporter.onMessage("完整分割: " + input_raster);
                int ret = seg.SegmentToFile(input_raster, output_tif, output_shp, config);
                reporter.onProgress(1.0);
                return ret == 0;
            } else if (actionKey == "segment_raster") {
                reporter.onMessage("仅输出栅格: " + input_raster);
                int ret = seg.SegmentToFile(input_raster, output_tif, "", config);
                reporter.onProgress(1.0);
                return ret == 0;
            } else if (actionKey == "segment_vector") {
                reporter.onMessage("仅输出矢量: " + input_raster);
                RasterIO rio;
                auto raster_data = rio.Load(input_raster);
                if (!raster_data) {
                    reporter.onMessage("无法加载影像文件: " + input_raster);
                    return false;
                }
                auto vector_data = seg.SegmentToPolygon(*raster_data, config);
                VectorIO vio;
                vio.Save(*vector_data, output_shp);
                reporter.onProgress(1.0);
                return true;
            }
        } else if (pluginName == "inference") {
            auto model_path = getStringParam(params, "model_path");
            auto input_raster = getStringParam(params, "input_raster");
            auto output_path = getStringParam(params, "output_path");
            auto config = buildInferenceConfig(params);

            LargeImageSeg seg(model_path);
            seg.SetProgressCallback([&reporter](int current, int total, const std::string& msg) {
                if (total > 0) {
                    reporter.onProgress(static_cast<double>(current) / total);
                }
                reporter.onMessage(msg);
            });

            if (actionKey == "inference_single") {
                reporter.onMessage("单图推理: " + input_raster);
                int ret = seg.SegmentToFile(input_raster, output_path, "", config);
                reporter.onProgress(1.0);
                return ret == 0;
            }
        } else if (pluginName == "preprocess") {
            auto input_raster = getStringParam(params, "input_raster");
            auto output_path = getStringParam(params, "output_path");

            RasterIO rio;
            reporter.onMessage("加载影像: " + input_raster);
            reporter.onProgress(0.1);
            auto raster_data = rio.Load(input_raster);
                if (!raster_data) {
                    reporter.onMessage("无法加载影像文件: " + input_raster);
                    return false;
                }

            if (actionKey == "preprocess_resample") {
                auto method_str = getStringParam(params, "resample_method");
                reporter.onMessage("重采样: " + method_str);

                ResampleMethod method = ResampleMethod::Nearest;
                if (method_str.find("Bilinear") != std::string::npos) method = ResampleMethod::Bilinear;

                int target_width = getIntParam(params, "resample_width", 0);
                int target_height = getIntParam(params, "resample_height", 0);
                if (target_width <= 0) target_width = raster_data->width;
                if (target_height <= 0) target_height = raster_data->height;

                RasterResample resample;
                auto result = resample.Execute(*raster_data, target_width, target_height, method);
                rio.Save(*result, output_path);
                reporter.onProgress(1.0);
                return true;
            } else if (actionKey == "preprocess_normalize") {
                auto mode_str = getStringParam(params, "normalize_mode");
                reporter.onMessage("归一化: " + mode_str);

                if (mode_str == "None") {
                    rio.Save(*raster_data, output_path);
                } else {
                    RasterNormalize normalize;
                    auto result = normalize.Execute(*raster_data);
                    rio.Save(*result, output_path);
                }
                reporter.onProgress(1.0);
                return true;
            } else if (actionKey == "preprocess_clip") {
                auto extent = getExtentParam(params, "clip_extent");
                reporter.onMessage("裁剪范围: " + std::to_string(extent[0]) + "," +
                    std::to_string(extent[1]) + "," + std::to_string(extent[2]) + "," + std::to_string(extent[3]));

                BoundingBox bbox;
                bbox.min_x = extent[0];
                bbox.min_y = extent[1];
                bbox.max_x = extent[2];
                bbox.max_y = extent[3];

                RasterClip clip;
                auto result = clip.Execute(*raster_data, bbox);
                rio.Save(*result, output_path);
                reporter.onProgress(1.0);
                return true;
            }
        } else if (pluginName == "vector") {
            auto input_vector = getStringParam(params, "input_vector");
            auto output_path = getStringParam(params, "output_path");

            VectorIO vio;
            reporter.onMessage("加载矢量: " + input_vector);
            reporter.onProgress(0.1);
            auto vector_data = vio.Load(input_vector);
                if (!vector_data) {
                    reporter.onMessage("无法加载矢量文件: " + input_vector);
                    return false;
                }

            if (actionKey == "vector_simplify") {
                auto tolerance = getDoubleParam(params, "simplify_tolerance", 1.0);
                reporter.onMessage("简化容差: " + std::to_string(tolerance));

                VectorSimplify simplify;
                auto result = simplify.Execute(*vector_data, tolerance);
                vio.Save(*result, output_path);
                reporter.onProgress(1.0);
                return true;
            } else if (actionKey == "vector_buffer") {
                auto distance = getDoubleParam(params, "buffer_distance", 10.0);
                reporter.onMessage("缓冲距离: " + std::to_string(distance));

                VectorBuffer buffer;
                auto result = buffer.Execute(*vector_data, distance);
                vio.Save(*result, output_path);
                reporter.onProgress(1.0);
                return true;
            } else if (actionKey == "vector_clip") {
                auto extent = getExtentParam(params, "clip_extent");
                reporter.onMessage("裁剪范围: " + std::to_string(extent[0]) + "," +
                    std::to_string(extent[1]) + "," + std::to_string(extent[2]) + "," + std::to_string(extent[3]));

                VectorData clipper;
                clipper.feature_type = FeatureType::Polygon;
                clipper.projection = vector_data->projection;

                Feature clip_feature;
                clip_feature.type = FeatureType::Polygon;
                clip_feature.coordinates = {
                    {extent[0], extent[1], 0.0},
                    {extent[2], extent[1], 0.0},
                    {extent[2], extent[3], 0.0},
                    {extent[0], extent[3], 0.0},
                    {extent[0], extent[1], 0.0},
                };
                clipper.features.push_back(std::move(clip_feature));

                VectorClip vclip;
                auto result = vclip.Execute(*vector_data, clipper);
                vio.Save(*result, output_path);
                reporter.onProgress(1.0);
                return true;
            }
        } else if (pluginName == "raster") {
            auto input_raster = getStringParam(params, "input_raster");
            auto output_path = getStringParam(params, "output_path");

            RasterIO rio;
            reporter.onMessage("加载栅格: " + input_raster);
            reporter.onProgress(0.1);
            auto raster_data = rio.Load(input_raster);
                if (!raster_data) {
                    reporter.onMessage("无法加载影像文件: " + input_raster);
                    return false;
                }

            if (actionKey == "raster_mosaic") {
                auto strategy_str = getStringParam(params, "mosaic_strategy");
                reporter.onMessage("镶嵌策略: " + strategy_str);

                MosaicStrategy strategy = MosaicStrategy::First;
                if (strategy_str.find("Overwrite") != std::string::npos) strategy = MosaicStrategy::Overwrite;
                else if (strategy_str.find("Mean") != std::string::npos) strategy = MosaicStrategy::Mean;
                else if (strategy_str.find("Max") != std::string::npos) strategy = MosaicStrategy::Max;
                else if (strategy_str.find("Min") != std::string::npos) strategy = MosaicStrategy::Min;

                MosaicConfig mosaic_config;
                mosaic_config.strategy = strategy;

                std::vector<std::unique_ptr<RasterData>> loaded;
                std::vector<std::reference_wrapper<const RasterData>> refs;

                auto addRaster = [&](const std::string& path) -> bool {
                    if (path.empty()) return true;
                    auto data = rio.Load(path);
                    if (!data) {
                        reporter.onMessage("无法加载影像文件: " + path);
                        return false;
                    }
                    refs.push_back(std::cref(*data));
                    loaded.push_back(std::move(data));
                    return true;
                };

                if (!addRaster(input_raster)) return false;

                std::string extra = getStringParam(params, "extra_rasters");
                if (!extra.empty()) {
                    std::istringstream stream(extra);
                    std::string token;
                    while (std::getline(stream, token, ';')) {
                        while (!token.empty() && (token.front() == ' ' || token.front() == '\t'))
                            token.erase(token.begin());
                        while (!token.empty() && (token.back() == ' ' || token.back() == '\t'))
                            token.pop_back();
                        if (!token.empty() && !addRaster(token)) return false;
                    }
                }

                if (refs.size() < 2) {
                    reporter.onMessage("镶嵌至少需要两个输入栅格，当前仅 " + std::to_string(refs.size()) + " 个");
                    return false;
                }

                reporter.onMessage("加载 " + std::to_string(refs.size()) + " 个栅格进行镶嵌");

                RasterMosaic mosaic;
                auto result = mosaic.Execute(refs, mosaic_config);
                rio.Save(*result, output_path);
                reporter.onProgress(1.0);
                return true;
            } else if (actionKey == "raster_threshold") {
                auto threshold = getDoubleParam(params, "threshold_value", 128.0);
                reporter.onMessage("阈值: " + std::to_string(threshold));

                RasterThreshold thresh;
                auto result = thresh.Execute(*raster_data, threshold);
                rio.Save(*result, output_path);
                reporter.onProgress(1.0);
                return true;
            } else if (actionKey == "raster_resample") {
                auto method_str = getStringParam(params, "resample_method");
                reporter.onMessage("重采样: " + method_str);

                ResampleMethod method = ResampleMethod::Nearest;
                if (method_str.find("Bilinear") != std::string::npos) method = ResampleMethod::Bilinear;

                int target_width = getIntParam(params, "resample_width", 0);
                int target_height = getIntParam(params, "resample_height", 0);
                if (target_width <= 0) target_width = raster_data->width;
                if (target_height <= 0) target_height = raster_data->height;

                RasterResample resample;
                auto result = resample.Execute(*raster_data, target_width, target_height, method);
                rio.Save(*result, output_path);
                reporter.onProgress(1.0);
                return true;
            }
        } else if (pluginName == "batch") {
            auto input_dir = getStringParam(params, "input_dir");
            auto model_path = getStringParam(params, "model_path");
            auto output_dir = getStringParam(params, "output_dir");

            LargeImageSeg seg(model_path);

            if (actionKey == "batch_segment" || actionKey == "batch_inference") {
                if (!std::filesystem::exists(input_dir)) {
                    reporter.onMessage("输入目录不存在: " + input_dir);
                    return false;
                }
                if (!std::filesystem::exists(output_dir)) {
                    std::filesystem::create_directories(output_dir);
                }

                std::vector<std::filesystem::path> raster_files;
                for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
                    auto ext = entry.path().extension().string();
                    if (ext == ".tif" || ext == ".tiff" || ext == ".img" || ext == ".png") {
                        raster_files.push_back(entry.path());
                    }
                }

                if (raster_files.empty()) {
                    reporter.onMessage("输入目录中未找到栅格文件");
                    return false;
                }

                reporter.onMessage("找到 " + std::to_string(raster_files.size()) + " 个栅格文件");

                seg.SetProgressCallback([&reporter, total = raster_files.size()](int current, int, const std::string& msg) {
                    reporter.onProgress(static_cast<double>(current) / total);
                    reporter.onMessage(msg);
                });

                int success_count = 0;
                for (size_t i = 0; i < raster_files.size(); ++i) {
                    if (reporter.isCancelled()) {
                        reporter.onMessage("任务已取消");
                        return false;
                    }

                    auto stem = raster_files[i].stem().string();
                    auto out_tif = std::filesystem::path(output_dir) / (stem + "_result.tif");

                    reporter.onMessage("处理: " + raster_files[i].string());

                    try {
                        const auto config = (actionKey == "batch_inference")
                            ? buildInferenceConfig(params)
                            : buildSegConfig(params);
                        int ret = seg.SegmentToFile(raster_files[i].string(), out_tif.string(), "", config);
                        if (ret == 0) success_count++;
                    } catch (const std::exception& e) {
                        reporter.onMessage("处理失败: " + std::string(e.what()));
                    }

                    reporter.onProgress(static_cast<double>(i + 1) / raster_files.size());
                }

                reporter.onMessage("批量处理完成: " + std::to_string(success_count) + "/" + std::to_string(raster_files.size()) + " 成功");
                return success_count > 0;
            }
        }

        reporter.onMessage("未知功能: " + pluginName + "/" + actionKey);
        return false;
    } catch (const std::exception& e) {
        reporter.onMessage("执行异常: " + std::string(e.what()));
        return false;
    }
}

std::string localizeResultMessage(const std::string& message) {
    static const std::map<std::string, std::string> kKnownMessages = {
        {"Segmentation completed successfully", "分割完成"},
        {"Batch segmentation completed", "批量分割完成"},
        {"Inference completed successfully", "推理完成"},
        {"Resample completed successfully", "重采样完成"},
        {"Normalization completed successfully", "归一化完成"},
        {"Clip completed successfully", "裁剪完成"},
        {"Vector simplify completed successfully", "矢量简化完成"},
        {"Vector buffer completed successfully", "缓冲区处理完成"},
        {"Vector clip completed successfully", "矢量裁剪完成"},
        {"Raster mosaic completed successfully", "栅格镶嵌完成"},
        {"Raster threshold completed successfully", "阈值分割完成"},
        {"Raster resample completed successfully", "栅格重采样完成"},
        {"Cancelled", "操作已取消"},
    };

    const auto it = kKnownMessages.find(message);
    if (it != kKnownMessages.end()) {
        return it->second;
    }

    std::string msg = message;

    if (msg.find("Cannot open") != std::string::npos ||
        msg.find("No such file") != std::string::npos) {
        return "无法打开文件：" + msg + "\n建议：请检查文件路径是否正确，文件是否存在。";
    }
    if (msg.find("Permission denied") != std::string::npos) {
        return "权限不足：" + msg + "\n建议：请检查文件是否被其他程序占用，或是否有写入权限。";
    }
    if (msg.find("out of memory") != std::string::npos ||
        msg.find("OutOfMemory") != std::string::npos) {
        return "内存不足：" + msg + "\n建议：请尝试处理更小的数据范围，或先分块处理。";
    }
    if (msg.find("Unsupported format") != std::string::npos ||
        msg.find("not recognized") != std::string::npos) {
        return "格式不支持：" + msg + "\n建议：请确认输入文件格式是否受支持。";
    }
    if (msg.find("Cancelled") != std::string::npos) {
        return "操作已取消";
    }

    return msg;
}

}
