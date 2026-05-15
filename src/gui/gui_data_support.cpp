#include "gui_data_support.h"
#include "qt_progress_reporter.h"

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

#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>

#include <filesystem>
#include <array>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <QFileInfo>

namespace gis_ai::gui {

namespace {

std::string getStringParam(const std::map<std::string, ParamValue>& params, const std::string& key) {
    auto it = params.find(key);
    if (it == params.end()) return {};
    if (auto* v = std::get_if<std::string>(&it->second)) return *v;
    return {};
}

int getIntParam(const std::map<std::string, ParamValue>& params, const std::string& key, int def = 0) {
    auto it = params.find(key);
    if (it == params.end()) return def;
    if (auto* v = std::get_if<int>(&it->second)) return *v;
    return def;
}

double getDoubleParam(const std::map<std::string, ParamValue>& params, const std::string& key, double def = 0.0) {
    auto it = params.find(key);
    if (it == params.end()) return def;
    if (auto* v = std::get_if<double>(&it->second)) return *v;
    return def;
}

bool getBoolParam(const std::map<std::string, ParamValue>& params, const std::string& key, bool def = false) {
    auto it = params.find(key);
    if (it == params.end()) return def;
    if (auto* v = std::get_if<bool>(&it->second)) return *v;
    return def;
}

std::array<double, 4> getExtentParam(const std::map<std::string, ParamValue>& params, const std::string& key) {
    auto it = params.find(key);
    if (it == params.end()) return {0, 0, 0, 0};
    if (auto* v = std::get_if<std::array<double, 4>>(&it->second)) return *v;
    return {0, 0, 0, 0};
}

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

}

std::vector<ParamSpec> getParamSpecsForPlugin(const std::string& pluginName) {
    std::vector<ParamSpec> specs;

    if (pluginName == "segment") {
        specs.push_back({"model_path", "模型路径", "ONNX 推理模型文件", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"input_raster", "输入影像", "待分割的栅格影像文件", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"output_tif", "输出栅格", "分割结果栅格输出路径", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"output_shp", "输出矢量", "分割结果矢量输出路径", ParamType::FilePath, false, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"tile_size", "分块大小", "大图分块推理的块尺寸", ParamType::Int, false, int{512}, int{64}, int{4096}, {}});
        specs.push_back({"stride", "步长", "分块推理的滑动步长", ParamType::Int, false, int{256}, int{32}, int{2048}, {}});
        specs.push_back({"blend_mode", "融合方式", "分块重叠区域的融合方式", ParamType::Enum, false, std::string{"Gaussian"}, int{0}, int{0}, {"None（无融合）", "Linear（线性融合）", "Gaussian（高斯融合）"}});
        specs.push_back({"target_class", "目标类别", "分割目标类别编号", ParamType::Int, false, int{1}, int{0}, int{255}, {}});
        specs.push_back({"skip_nodata", "跳过无数据", "是否跳过无数据区域", ParamType::Bool, false, bool{true}, int{0}, int{0}, {}});
    } else if (pluginName == "inference") {
        specs.push_back({"model_path", "模型路径", "ONNX 推理模型文件", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"input_raster", "输入影像", "待推理的栅格影像文件", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"output_path", "输出路径", "推理结果输出路径", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"target_class", "目标类别", "推理目标类别编号", ParamType::Int, false, int{1}, int{0}, int{255}, {}});
    } else if (pluginName == "preprocess") {
        specs.push_back({"input_raster", "输入影像", "待预处理的栅格影像文件", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"output_path", "输出路径", "预处理结果输出路径", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"resample_method", "重采样方式", "影像重采样方法", ParamType::Enum, false, std::string{"Nearest（最邻近）"}, int{0}, int{0}, {"Nearest（最邻近）", "Bilinear（双线性）"}});
        specs.push_back({"normalize_mode", "归一化方式", "影像归一化处理方式", ParamType::Enum, false, std::string{"None（不处理）"}, int{0}, int{0}, {"MinMax（最小最大）", "ZScore（标准差）", "None（不处理）"}});
        specs.push_back({"clip_extent", "裁剪范围", "影像裁剪范围 (Xmin, Ymin, Xmax, Ymax)", ParamType::Extent, false, std::array<double, 4>{0, 0, 0, 0}, int{0}, int{0}, {}});
    } else if (pluginName == "vector") {
        specs.push_back({"input_vector", "输入矢量", "待处理的矢量文件", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"output_path", "输出路径", "处理结果输出路径", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"simplify_tolerance", "简化容差", "矢量简化的容差值", ParamType::Double, false, double{1.0}, double{0.0}, double{1000.0}, {}});
        specs.push_back({"buffer_distance", "缓冲距离", "矢量缓冲区距离", ParamType::Double, false, double{10.0}, double{0.0}, double{100000.0}, {}});
        specs.push_back({"clip_extent", "裁剪范围", "矢量裁剪范围 (Xmin, Ymin, Xmax, Ymax)", ParamType::Extent, false, std::array<double, 4>{0, 0, 0, 0}, int{0}, int{0}, {}});
    } else if (pluginName == "raster") {
        specs.push_back({"input_raster", "输入栅格", "待处理的栅格影像文件", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"output_path", "输出路径", "处理结果输出路径", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"mosaic_strategy", "镶嵌策略", "栅格镶嵌合并策略", ParamType::Enum, false, std::string{"First（首个）"}, int{0}, int{0}, {"First（首个）", "Overwrite（覆盖）", "Mean（均值）", "Max（最大值）", "Min（最小值）"}});
        specs.push_back({"threshold_value", "阈值", "栅格阈值分割的阈值", ParamType::Double, false, double{128.0}, double{0.0}, double{65535.0}, {}});
        specs.push_back({"resample_method", "重采样方式", "栅格重采样方法", ParamType::Enum, false, std::string{"Nearest（最邻近）"}, int{0}, int{0}, {"Nearest（最邻近）", "Bilinear（双线性）"}});
    } else if (pluginName == "batch") {
        specs.push_back({"input_dir", "输入目录", "批量处理的输入目录", ParamType::DirPath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"model_path", "模型路径", "ONNX 推理模型文件", ParamType::FilePath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"output_dir", "输出目录", "批量处理的输出目录", ParamType::DirPath, true, std::string{}, int{0}, int{0}, {}});
        specs.push_back({"tile_size", "分块大小", "大图分块推理的块尺寸", ParamType::Int, false, int{512}, int{64}, int{4096}, {}});
        specs.push_back({"stride", "步长", "分块推理的滑动步长", ParamType::Int, false, int{256}, int{32}, int{2048}, {}});
        specs.push_back({"blend_mode", "融合方式", "分块重叠区域的融合方式", ParamType::Enum, false, std::string{"Gaussian（高斯融合）"}, int{0}, int{0}, {"None（无融合）", "Linear（线性融合）", "Gaussian（高斯融合）"}});
    }

    return specs;
}

ActionUiConfig getActionUiConfig(const std::string& pluginName, const std::string& actionKey) {
    ActionUiConfig cfg;

    if (pluginName == "segment") {
        if (actionKey == "segment_full") {
            cfg.displayName = QStringLiteral("完整分割");
            cfg.description = QStringLiteral("对大图进行分割，同时输出栅格和矢量结果");
            cfg.visibleKeys = {"model_path", "input_raster", "output_tif", "output_shp", "tile_size", "stride", "blend_mode", "target_class", "skip_nodata"};
            cfg.requiredKeys = {"model_path", "input_raster", "output_tif"};
        } else if (actionKey == "segment_raster") {
            cfg.displayName = QStringLiteral("仅输出栅格");
            cfg.description = QStringLiteral("对大图进行分割，仅输出栅格结果");
            cfg.visibleKeys = {"model_path", "input_raster", "output_tif", "tile_size", "stride", "blend_mode", "target_class", "skip_nodata"};
            cfg.requiredKeys = {"model_path", "input_raster", "output_tif"};
        } else if (actionKey == "segment_vector") {
            cfg.displayName = QStringLiteral("仅输出矢量");
            cfg.description = QStringLiteral("对大图进行分割，仅输出矢量结果");
            cfg.visibleKeys = {"model_path", "input_raster", "output_shp", "tile_size", "stride", "blend_mode", "target_class", "skip_nodata"};
            cfg.requiredKeys = {"model_path", "input_raster", "output_shp"};
        }
    } else if (pluginName == "inference") {
        if (actionKey == "inference_single") {
            cfg.displayName = QStringLiteral("单图推理");
            cfg.description = QStringLiteral("对单张影像进行模型推理");
            cfg.visibleKeys = {"model_path", "input_raster", "output_path", "target_class"};
            cfg.requiredKeys = {"model_path", "input_raster", "output_path"};
        } else if (actionKey == "inference_batch") {
            cfg.displayName = QStringLiteral("批量推理");
            cfg.description = QStringLiteral("对目录下所有影像进行批量推理");
            cfg.visibleKeys = {"model_path", "input_raster", "output_path", "target_class"};
            cfg.requiredKeys = {"model_path", "input_raster", "output_path"};
        }
    } else if (pluginName == "preprocess") {
        if (actionKey == "preprocess_resample") {
            cfg.displayName = QStringLiteral("重采样");
            cfg.description = QStringLiteral("对栅格影像进行重采样处理");
            cfg.visibleKeys = {"input_raster", "output_path", "resample_method"};
            cfg.requiredKeys = {"input_raster", "output_path"};
        } else if (actionKey == "preprocess_normalize") {
            cfg.displayName = QStringLiteral("归一化");
            cfg.description = QStringLiteral("对栅格影像进行归一化处理");
            cfg.visibleKeys = {"input_raster", "output_path", "normalize_mode"};
            cfg.requiredKeys = {"input_raster", "output_path"};
        } else if (actionKey == "preprocess_clip") {
            cfg.displayName = QStringLiteral("裁剪");
            cfg.description = QStringLiteral("按范围裁剪栅格影像");
            cfg.visibleKeys = {"input_raster", "output_path", "clip_extent"};
            cfg.requiredKeys = {"input_raster", "output_path"};
        }
    } else if (pluginName == "vector") {
        if (actionKey == "vector_simplify") {
            cfg.displayName = QStringLiteral("简化");
            cfg.description = QStringLiteral("简化矢量要素，减少顶点数量");
            cfg.visibleKeys = {"input_vector", "output_path", "simplify_tolerance"};
            cfg.requiredKeys = {"input_vector", "output_path"};
        } else if (actionKey == "vector_buffer") {
            cfg.displayName = QStringLiteral("缓冲区");
            cfg.description = QStringLiteral("为矢量要素创建缓冲区");
            cfg.visibleKeys = {"input_vector", "output_path", "buffer_distance"};
            cfg.requiredKeys = {"input_vector", "output_path"};
        } else if (actionKey == "vector_clip") {
            cfg.displayName = QStringLiteral("裁剪");
            cfg.description = QStringLiteral("按范围裁剪矢量数据");
            cfg.visibleKeys = {"input_vector", "output_path", "clip_extent"};
            cfg.requiredKeys = {"input_vector", "output_path"};
        }
    } else if (pluginName == "raster") {
        if (actionKey == "raster_mosaic") {
            cfg.displayName = QStringLiteral("镶嵌");
            cfg.description = QStringLiteral("将多个栅格影像镶嵌合并");
            cfg.visibleKeys = {"input_raster", "output_path", "mosaic_strategy"};
            cfg.requiredKeys = {"input_raster", "output_path"};
        } else if (actionKey == "raster_threshold") {
            cfg.displayName = QStringLiteral("阈值分割");
            cfg.description = QStringLiteral("对栅格影像进行阈值分割");
            cfg.visibleKeys = {"input_raster", "output_path", "threshold_value"};
            cfg.requiredKeys = {"input_raster", "output_path"};
        } else if (actionKey == "raster_resample") {
            cfg.displayName = QStringLiteral("重采样");
            cfg.description = QStringLiteral("对栅格影像进行重采样处理");
            cfg.visibleKeys = {"input_raster", "output_path", "resample_method"};
            cfg.requiredKeys = {"input_raster", "output_path"};
        }
    } else if (pluginName == "batch") {
        if (actionKey == "batch_segment") {
            cfg.displayName = QStringLiteral("批量分割");
            cfg.description = QStringLiteral("对目录下所有影像进行批量分割");
            cfg.visibleKeys = {"input_dir", "model_path", "output_dir", "tile_size", "stride", "blend_mode"};
            cfg.requiredKeys = {"input_dir", "model_path", "output_dir"};
        } else if (actionKey == "batch_inference") {
            cfg.displayName = QStringLiteral("批量推理");
            cfg.description = QStringLiteral("对目录下所有影像进行批量推理");
            cfg.visibleKeys = {"input_dir", "model_path", "output_dir", "tile_size", "stride", "blend_mode"};
            cfg.requiredKeys = {"input_dir", "model_path", "output_dir"};
        }
    }

    return cfg;
}

std::vector<ParamSpec> buildEffectiveParamSpecs(const std::string& pluginName, const std::string& actionKey) {
    auto allSpecs = getParamSpecsForPlugin(pluginName);
    auto uiConfig = getActionUiConfig(pluginName, actionKey);

    std::vector<ParamSpec> result;
    for (const auto& spec : allSpecs) {
        if (uiConfig.visibleKeys.count(spec.key)) {
            ParamSpec effective = spec;
            effective.required = uiConfig.requiredKeys.count(spec.key) > 0;
            result.push_back(effective);
        }
    }
    return result;
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
            auto target_class = getIntParam(params, "target_class", 1);

            LargeImageSegConfig config;
            config.target_class = static_cast<uint8_t>(target_class);

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
            } else if (actionKey == "inference_batch") {
                reporter.onMessage("批量推理: " + input_raster);
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

            if (actionKey == "preprocess_resample") {
                auto method_str = getStringParam(params, "resample_method");
                reporter.onMessage("重采样: " + method_str);

                ResampleMethod method = ResampleMethod::Nearest;
                if (method_str.find("Bilinear") != std::string::npos) method = ResampleMethod::Bilinear;

                int new_size = std::max(raster_data->width, raster_data->height);
                RasterResample resample;
                auto result = resample.Execute(*raster_data, new_size, new_size, method);
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

                BoundingBox bbox;
                bbox.min_x = extent[0];
                bbox.min_y = extent[1];
                bbox.max_x = extent[2];
                bbox.max_y = extent[3];

                RasterIO rio;
                RasterClip raster_clip;
                auto clip_raster = std::make_unique<RasterData>();
                clip_raster->width = 1;
                clip_raster->height = 1;
                clip_raster->band_count = 1;
                clip_raster->geotransform[0] = bbox.min_x;
                clip_raster->geotransform[1] = 1.0;
                clip_raster->geotransform[3] = bbox.max_y;
                clip_raster->geotransform[5] = -1.0;
                clip_raster->bands.resize(1);
                clip_raster->bands[0].resize(1, 1.0f);

                VectorClip vclip;
                auto result = vclip.Execute(*vector_data, *vector_data);
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

                RasterMosaic mosaic;
                std::vector<std::reference_wrapper<const RasterData>> refs;
                refs.push_back(std::cref(*raster_data));
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

                int new_size = std::max(raster_data->width, raster_data->height);
                RasterResample resample;
                auto result = resample.Execute(*raster_data, new_size, new_size, method);
                rio.Save(*result, output_path);
                reporter.onProgress(1.0);
                return true;
            }
        } else if (pluginName == "batch") {
            auto input_dir = getStringParam(params, "input_dir");
            auto model_path = getStringParam(params, "model_path");
            auto output_dir = getStringParam(params, "output_dir");
            auto config = buildSegConfig(params);

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

std::optional<ActionValidationIssue> validateActionSpecificParams(
    const std::string& pluginName,
    const std::string& actionKey,
    const std::map<std::string, ParamValue>& params) {
    auto stringParam = [&](const std::string& key) -> std::string {
        const auto it = params.find(key);
        if (it == params.end()) return {};
        if (const auto* value = std::get_if<std::string>(&it->second)) return *value;
        return {};
    };

    auto intParam = [&](const std::string& key) -> std::optional<int> {
        const auto it = params.find(key);
        if (it == params.end()) return std::nullopt;
        if (const auto* value = std::get_if<int>(&it->second)) return *value;
        return std::nullopt;
    };

    auto doubleParam = [&](const std::string& key) -> std::optional<double> {
        const auto it = params.find(key);
        if (it == params.end()) return std::nullopt;
        if (const auto* value = std::get_if<double>(&it->second)) return *value;
        if (const auto* value = std::get_if<int>(&it->second)) return static_cast<double>(*value);
        return std::nullopt;
    };

    if (pluginName == "segment") {
        const auto tileSize = intParam("tile_size");
        if (tileSize.has_value() && *tileSize < 64) {
            return ActionValidationIssue{"tile_size", QString::fromUtf8("参数「分块大小」不能小于 64")};
        }
        const auto stride = intParam("stride");
        if (stride.has_value() && *stride < 32) {
            return ActionValidationIssue{"stride", QString::fromUtf8("参数「步长」不能小于 32")};
        }
        if (tileSize.has_value() && stride.has_value() && *stride > *tileSize) {
            return ActionValidationIssue{"stride", QString::fromUtf8("参数「步长」不应大于「分块大小」")};
        }
        const auto targetClass = intParam("target_class");
        if (targetClass.has_value() && (*targetClass < 0 || *targetClass > 255)) {
            return ActionValidationIssue{"target_class", QString::fromUtf8("参数「目标类别」应落在 [0, 255] 范围内")};
        }
        const std::string outputTif = stringParam("output_tif");
        if (!outputTif.empty() && outputTif.find(".tif") == std::string::npos &&
            outputTif.find(".tiff") == std::string::npos) {
            return ActionValidationIssue{"output_tif", QString::fromUtf8("参数「输出栅格」应使用 .tif 或 .tiff")};
        }
        const std::string outputShp = stringParam("output_shp");
        if (!outputShp.empty() && outputShp.find(".shp") == std::string::npos &&
            outputShp.find(".gpkg") == std::string::npos &&
            outputShp.find(".geojson") == std::string::npos) {
            return ActionValidationIssue{"output_shp", QString::fromUtf8("参数「输出矢量」应使用 .shp、.gpkg 或 .geojson")};
        }
    }

    if (pluginName == "inference") {
        const auto targetClass = intParam("target_class");
        if (targetClass.has_value() && (*targetClass < 0 || *targetClass > 255)) {
            return ActionValidationIssue{"target_class", QString::fromUtf8("参数「目标类别」应落在 [0, 255] 范围内")};
        }
    }

    if (pluginName == "preprocess") {
        if (actionKey == "preprocess_clip") {
            const auto extent = getExtentParam(params, "clip_extent");
            if (extent[0] == 0 && extent[1] == 0 && extent[2] == 0 && extent[3] == 0) {
                return ActionValidationIssue{"clip_extent", QString::fromUtf8("参数「裁剪范围」不能全为零")};
            }
        }
    }

    if (pluginName == "vector") {
        if (actionKey == "vector_simplify") {
            const auto tolerance = doubleParam("simplify_tolerance");
            if (tolerance.has_value() && *tolerance <= 0.0) {
                return ActionValidationIssue{"simplify_tolerance", QString::fromUtf8("参数「简化容差」必须大于 0")};
            }
        }
        if (actionKey == "vector_buffer") {
            const auto distance = doubleParam("buffer_distance");
            if (distance.has_value() && *distance <= 0.0) {
                return ActionValidationIssue{"buffer_distance", QString::fromUtf8("参数「缓冲距离」必须大于 0")};
            }
        }
    }

    if (pluginName == "raster") {
        if (actionKey == "raster_threshold") {
            const auto threshold = doubleParam("threshold_value");
            if (threshold.has_value() && *threshold < 0.0) {
                return ActionValidationIssue{"threshold_value", QString::fromUtf8("参数「阈值」不能小于 0")};
            }
        }
    }

    if (pluginName == "batch") {
        const std::string inputDir = stringParam("input_dir");
        if (!inputDir.empty() && !std::filesystem::exists(inputDir)) {
            return ActionValidationIssue{"input_dir", QString::fromUtf8("参数「输入目录」路径不存在")};
        }
    }

    return std::nullopt;
}

ExecuteButtonState buildExecuteButtonState(bool hasSelection,
                                           const QString& validationMessage) {
    if (!hasSelection) {
        return ExecuteButtonState{
            false,
            QStringLiteral("请先选择主功能和子功能"),
            QStringLiteral("就绪"),
            QStringLiteral("statusBadgeReady")
        };
    }
    if (!validationMessage.isEmpty()) {
        return ExecuteButtonState{
            false,
            validationMessage,
            QStringLiteral("待修正"),
            QStringLiteral("statusBadgeWarning")
        };
    }
    return ExecuteButtonState{
        true,
        QStringLiteral("参数已就绪，可以执行当前功能"),
        QStringLiteral("可执行"),
        QStringLiteral("statusBadgeReady")
    };
}

namespace {

std::string findFirstInvalidParamKeyLocal(
    const std::vector<ParamSpec>& specs,
    const std::map<std::string, ParamValue>& params) {
    for (const auto& spec : specs) {
        if (!spec.required) continue;

        const auto it = params.find(spec.key);
        if (it == params.end()) {
            return spec.key;
        }

        if (spec.type == ParamType::FilePath || spec.type == ParamType::DirPath) {
            const auto* str = std::get_if<std::string>(&it->second);
            if (!str || str->empty()) {
                return spec.key;
            }

            const bool isOutput = spec.key.find("output") != std::string::npos;
            if (!isOutput) {
                const QFileInfo fi(QString::fromStdString(*str));
                if (!fi.exists()) {
                    return spec.key;
                }
            } else {
                const QFileInfo fi(QString::fromStdString(*str));
                const QFileInfo parentDir(fi.absolutePath());
                if (!parentDir.exists() || !parentDir.isDir()) {
                    return spec.key;
                }
            }
        }

        if (spec.type == ParamType::FilePath && spec.key.find("model") != std::string::npos) {
            const auto* str = std::get_if<std::string>(&it->second);
            if (str && !str->empty()) {
                const QFileInfo fi(QString::fromStdString(*str));
                if (!fi.exists()) {
                    return spec.key;
                }
            }
        }
    }
    return {};
}

}

std::string resolveHighlightedParamKey(
    bool hasSelection,
    const std::vector<ParamSpec>& specs,
    const std::map<std::string, ParamValue>& params,
    const std::optional<ActionValidationIssue>& actionIssue) {
    if (!hasSelection) {
        return {};
    }
    const std::string invalidKey = findFirstInvalidParamKeyLocal(specs, params);
    if (!invalidKey.empty()) {
        return invalidKey;
    }
    if (actionIssue.has_value()) {
        return actionIssue->key;
    }
    return {};
}

namespace {

std::string lowerExtension(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return ext;
}

std::string sanitizeSuffixPart(const std::string& value) {
    std::string sanitized;
    sanitized.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch)) {
            sanitized.push_back(static_cast<char>(std::tolower(ch)));
        } else if (ch == '-' || ch == '_') {
            sanitized.push_back('_');
        }
    }
    return sanitized;
}

std::string firstInputPath(const std::string& rawPath) {
    const auto pos = rawPath.find(',');
    if (pos == std::string::npos) {
        auto trimmed = rawPath;
        while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
            trimmed.erase(trimmed.begin());
        while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t'))
            trimmed.pop_back();
        return trimmed;
    }
    auto trimmed = rawPath.substr(0, pos);
    while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
        trimmed.erase(trimmed.begin());
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t'))
        trimmed.pop_back();
    return trimmed;
}

std::string defaultSuffixForOutput(const std::string& pluginName,
                                   const std::string& action,
                                   const std::string& paramKey,
                                   const std::string& inputExt) {
    if (pluginName == "segment") {
        if (paramKey == "output_tif") return ".tif";
        if (paramKey == "output_shp") return ".shp";
        return inputExt;
    }
    if (pluginName == "inference") return ".tif";
    if (pluginName == "preprocess") return ".tif";
    if (pluginName == "vector") {
        if (action == "vector_simplify" || action == "vector_buffer" || action == "vector_clip")
            return ".gpkg";
        return inputExt;
    }
    if (pluginName == "raster") return ".tif";
    if (pluginName == "batch") return {};
    return inputExt;
}

std::string spatialReferenceText(const OGRSpatialReference* srs) {
    if (!srs) return {};
    const char* authName = srs->GetAuthorityName(nullptr);
    const char* authCode = srs->GetAuthorityCode(nullptr);
    if (authName && authCode) {
        return std::string(authName) + ":" + authCode;
    }
    char* wkt = nullptr;
    OGRSpatialReference cloned(*srs);
    if (cloned.exportToWkt(&wkt) != OGRERR_NONE || !wkt) return {};
    std::string text = wkt;
    CPLFree(wkt);
    return text;
}

struct DatasetCloser {
    void operator()(GDALDataset* ds) const {
        if (ds) GDALClose(ds);
    }
};

const std::map<std::string, ParamText>& commonParamTextStorage() {
    static const std::map<std::string, ParamText> kTexts = {
        {"model_path", {QStringLiteral("模型路径"), QStringLiteral("ONNX inference model file.")}},
        {"input_raster", {QStringLiteral("输入影像"), QStringLiteral("Input raster data path.")}},
        {"input_vector", {QStringLiteral("输入矢量"), QStringLiteral("Input vector data path.")}},
        {"output_tif", {QStringLiteral("输出栅格"), QStringLiteral("Output raster result path.")}},
        {"output_shp", {QStringLiteral("输出矢量"), QStringLiteral("Output vector result path.")}},
        {"output_path", {QStringLiteral("输出路径"), QStringLiteral("Output result path.")}},
        {"output_dir", {QStringLiteral("输出目录"), QStringLiteral("Output directory for batch results.")}},
        {"input_dir", {QStringLiteral("输入目录"), QStringLiteral("Input directory for batch processing.")}},
        {"tile_size", {QStringLiteral("分块大小"), QStringLiteral("Tile size for large image segmentation.")}},
        {"stride", {QStringLiteral("步长"), QStringLiteral("Stride for sliding window inference.")}},
        {"blend_mode", {QStringLiteral("融合方式"), QStringLiteral("Blend mode for tile overlap regions.")}},
        {"target_class", {QStringLiteral("目标类别"), QStringLiteral("Target class index for segmentation.")}},
        {"skip_nodata", {QStringLiteral("跳过无数据"), QStringLiteral("Skip NoData regions during inference.")}},
        {"simplify_tolerance", {QStringLiteral("简化容差"), QStringLiteral("Tolerance for vector simplification.")}},
        {"buffer_distance", {QStringLiteral("缓冲距离"), QStringLiteral("Buffer distance for vector buffering.")}},
        {"threshold_value", {QStringLiteral("阈值"), QStringLiteral("Threshold value for segmentation.")}},
        {"resample_method", {QStringLiteral("重采样方式"), QStringLiteral("Resampling method.")}},
        {"mosaic_strategy", {QStringLiteral("镶嵌策略"), QStringLiteral("Mosaic strategy for raster merging.")}},
        {"normalize_mode", {QStringLiteral("归一化方式"), QStringLiteral("Normalization method.")}},
        {"clip_extent", {QStringLiteral("裁剪范围"), QStringLiteral("Clipping extent (Xmin, Ymin, Xmax, Ymax).")}},
    };
    return kTexts;
}

}

DataKind detectDataKind(const std::string& path) {
    static const std::unordered_set<std::string> rasterExts = {
        ".tif", ".tiff", ".img", ".vrt", ".png", ".jpg", ".jpeg", ".bmp"
    };
    static const std::unordered_set<std::string> vectorExts = {
        ".shp", ".geojson", ".json", ".gpkg", ".kml", ".csv"
    };

    const std::string ext = lowerExtension(path);
    if (rasterExts.count(ext) > 0) return DataKind::Raster;
    if (vectorExts.count(ext) > 0) return DataKind::Vector;
    return DataKind::Unknown;
}

bool isSupportedDataPath(const std::string& path) {
    return detectDataKind(path) != DataKind::Unknown;
}

std::string dataKindDisplayName(DataKind kind) {
    switch (kind) {
        case DataKind::Raster: return "栅格";
        case DataKind::Vector: return "矢量";
        default: return "未知";
    }
}

DataAutoFillInfo inspectDataForAutoFill(const std::string& path) {
    DataAutoFillInfo info;
    const std::string normalizedPath = firstInputPath(path);
    const DataKind kind = detectDataKind(normalizedPath);

    if (kind == DataKind::Raster) {
        std::unique_ptr<GDALDataset, DatasetCloser> ds(
            static_cast<GDALDataset*>(GDALOpen(normalizedPath.c_str(), GA_ReadOnly)));
        if (!ds) return info;

        info.crs = spatialReferenceText(ds->GetSpatialRef());

        double gt[6] = {};
        if (ds->GetGeoTransform(gt) == CE_None) {
            const double minX = gt[0];
            const double maxY = gt[3];
            const double maxX = gt[0] + gt[1] * ds->GetRasterXSize() + gt[2] * ds->GetRasterYSize();
            const double minY = gt[3] + gt[4] * ds->GetRasterXSize() + gt[5] * ds->GetRasterYSize();
            info.extent = {
                std::min(minX, maxX),
                std::min(minY, maxY),
                std::max(minX, maxX),
                std::max(minY, maxY)
            };
            info.hasExtent = true;
        }
        return info;
    }

    if (kind == DataKind::Vector) {
        std::unique_ptr<GDALDataset, DatasetCloser> ds(
            static_cast<GDALDataset*>(GDALOpenEx(normalizedPath.c_str(),
                GDAL_OF_VECTOR | GDAL_OF_READONLY, nullptr, nullptr, nullptr)));
        if (!ds) return info;

        auto* layer = ds->GetLayer(0);
        if (!layer) return info;

        if (lowerExtension(normalizedPath) != ".shp") {
            info.layerName = layer->GetName();
        }
        info.crs = spatialReferenceText(layer->GetSpatialRef());

        OGREnvelope envelope{};
        if (layer->GetExtent(&envelope, TRUE) == OGRERR_NONE) {
            info.extent = {envelope.MinX, envelope.MinY, envelope.MaxX, envelope.MaxY};
            info.hasExtent = true;
        }
    }

    return info;
}

std::string buildSuggestedOutputPath(const std::string& inputPath,
                                     const std::string& pluginName,
                                     const std::string& action,
                                     const std::string& paramKey) {
    namespace fs = std::filesystem;

    fs::path input = fs::path(firstInputPath(inputPath));
    if (input.empty()) return {};

    const std::string pluginSuffix = sanitizeSuffixPart(pluginName);
    const std::string actionSuffix = sanitizeSuffixPart(action);

    std::string suffix = "result";
    if (!pluginSuffix.empty()) {
        suffix = pluginSuffix;
        if (!actionSuffix.empty()) suffix += "_" + actionSuffix;
    } else if (!actionSuffix.empty()) {
        suffix = actionSuffix;
    }

    std::string inputExtLower = lowerExtension(input.extension().string());
    const std::string resolvedSuffix = defaultSuffixForOutput(
        pluginName, action, paramKey, inputExtLower);

    if (resolvedSuffix.empty()) {
        return (input.parent_path() / fs::path(input.stem().string() + "_" + suffix)).generic_string();
    }

    const fs::path suggested = input.parent_path() /
        fs::path(input.stem().string() + "_" + suffix + input.extension().string());

    fs::path rewritten = suggested;
    rewritten.replace_extension(resolvedSuffix);
    return rewritten.generic_string();
}

DerivedOutputUpdate computeDerivedOutputUpdate(const std::string& currentValue,
                                               const std::string& lastAutoValue,
                                               const std::string& primaryPath,
                                               const std::string& pluginName,
                                               const std::string& action,
                                               const std::string& paramKey) {
    DerivedOutputUpdate update;
    if (primaryPath.empty()) return update;

    std::string suggestedValue = buildSuggestedOutputPath(primaryPath, pluginName, action, paramKey);

    const bool valueWasAuto = !lastAutoValue.empty() && currentValue == lastAutoValue;
    update.value = suggestedValue;
    update.autoValue = suggestedValue;
    update.shouldApply = (currentValue.empty() || valueWasAuto) && currentValue != suggestedValue;
    return update;
}

bool shouldAutoFillLayerValue(const std::string& currentValue,
                              const std::string& lastAutoValue,
                              const std::string& suggestedValue) {
    if (suggestedValue.empty()) return false;
    const bool valueWasAuto = !lastAutoValue.empty() && currentValue == lastAutoValue;
    return (currentValue.empty() || valueWasAuto) && currentValue != suggestedValue;
}

bool shouldAutoFillExtentValue(const std::optional<std::array<double, 4>>& currentValue,
                               const std::optional<std::array<double, 4>>& lastAutoValue,
                               bool hasSuggestedExtent) {
    if (!hasSuggestedExtent) return false;
    auto isZero = [](const std::array<double, 4>& e) -> bool {
        return e[0] == 0.0 && e[1] == 0.0 && e[2] == 0.0 && e[3] == 0.0;
    };
    const bool extentWasAuto = currentValue.has_value() && lastAutoValue.has_value()
        && *currentValue == *lastAutoValue;
    bool currentIsZeroOrEmpty = !currentValue.has_value() ||
        (currentValue.has_value() && isZero(*currentValue));
    return currentIsZeroOrEmpty || extentWasAuto;
}

FileParamUiConfig buildFileParamUiConfig(const std::string& pluginName,
                                         const std::string& action,
                                         const std::string& paramKey,
                                         ParamType paramType) {
    FileParamUiConfig config;
    config.isOutput = paramKey.find("output") != std::string::npos;

    if (paramType == ParamType::CRS) {
        config.placeholder = QString::fromUtf8("请输入 EPSG 代码，例如 EPSG:3857");
        return config;
    }

    if (config.isOutput) {
        config.suggestedSuffix = defaultSuffixForOutput(pluginName, action, paramKey, ".tif");

        if (config.suggestedSuffix == ".tif" || config.suggestedSuffix == ".tiff") {
            config.placeholder = QString::fromUtf8("请选择输出文件，建议使用 .tif");
            config.saveFilter = QStringLiteral("GeoTIFF (*.tif *.tiff);;所有文件 (*)");
        } else if (config.suggestedSuffix == ".shp") {
            config.placeholder = QString::fromUtf8("请选择输出文件，建议使用 .shp、.gpkg 或 .geojson");
            config.saveFilter = QStringLiteral("Shapefile (*.shp);;GeoPackage (*.gpkg);;GeoJSON (*.geojson);;所有文件 (*)");
        } else if (config.suggestedSuffix == ".gpkg") {
            config.placeholder = QString::fromUtf8("请选择输出文件，建议使用 .gpkg");
            config.saveFilter = QStringLiteral("GeoPackage (*.gpkg);;所有文件 (*)");
        } else if (config.suggestedSuffix == ".json") {
            config.placeholder = QString::fromUtf8("请选择输出文件，建议使用 .json");
            config.saveFilter = QStringLiteral("JSON 文件 (*.json);;所有文件 (*)");
        } else {
            config.placeholder = QString::fromUtf8("请选择输出文件");
            config.saveFilter = QStringLiteral("所有文件 (*)");
        }

        if (pluginName == "batch" && (paramKey == "output_dir" || paramKey == "input_dir")) {
            config.selectDirectory = true;
            config.isOutput = (paramKey == "output_dir");
            config.placeholder = QString::fromUtf8("请选择目录");
        }

        return config;
    }

    if (paramType == ParamType::DirPath) {
        config.selectDirectory = true;
        config.placeholder = QString::fromUtf8("请选择目录");
        return config;
    }

    if (paramKey == "model_path") {
        config.placeholder = QString::fromUtf8("请选择 ONNX 模型文件");
        config.openFilter = QStringLiteral("ONNX 模型 (*.onnx);;所有文件 (*)");
        return config;
    }

    if (paramKey.find("raster") != std::string::npos || paramKey == "input_raster") {
        config.placeholder = QString::fromUtf8("请选择栅格文件，例如 .tif、.img、.vrt");
        config.openFilter = QStringLiteral("栅格文件 (*.tif *.tiff *.img *.vrt *.png *.jpg *.jpeg *.bmp);;GeoTIFF (*.tif *.tiff);;所有文件 (*)");
        return config;
    }

    if (paramKey.find("vector") != std::string::npos || paramKey == "input_vector") {
        config.placeholder = QString::fromUtf8("请选择矢量文件，例如 .gpkg、.shp、.geojson");
        config.openFilter = QStringLiteral("矢量文件 (*.gpkg *.shp *.geojson *.json *.kml *.csv);;GeoPackage (*.gpkg);;Shapefile (*.shp);;GeoJSON (*.geojson *.json);;所有文件 (*)");
        return config;
    }

    config.placeholder = QString::fromUtf8("请选择文件或输入路径");
    return config;
}

const ParamText* findCommonParamText(const std::string& paramKey) {
    const auto& all = commonParamTextStorage();
    const auto it = all.find(paramKey);
    if (it == all.end()) return nullptr;
    return &it->second;
}

QString enumDisplayText(const std::string& paramKey, const std::string& rawValue) {
    static const std::map<std::string, std::map<std::string, QString>> translations = {
        {"method", {
            {"basic", QStringLiteral("basic (基础)")},
            {"otsu", QStringLiteral("otsu (大津法)")},
            {"adaptive", QStringLiteral("adaptive (自适应)")},
        }},
        {"match_method", {
            {"exact", QStringLiteral("exact (精确匹配)")},
            {"partial", QStringLiteral("partial (部分匹配)")},
            {"fuzzy", QStringLiteral("fuzzy (模糊匹配)")},
        }},
        {"transform", {
            {"identity", QStringLiteral("identity (无变换)")},
            {"normalize", QStringLiteral("normalize (归一化)")},
            {"standardize", QStringLiteral("standardize (标准化)")},
        }},
        {"resample", {
            {"nearest", QStringLiteral("nearest (最近邻)")},
            {"bilinear", QStringLiteral("bilinear (双线性)")},
            {"cubic", QStringLiteral("cubic (三次卷积)")},
            {"lanczos", QStringLiteral("lanczos (兰佐斯)")},
            {"average", QStringLiteral("average (平均值)")},
            {"mode", QStringLiteral("mode (众数)")},
        }},
        {"blend", {
            {"none", QStringLiteral("none (无融合)")},
            {"linear", QStringLiteral("linear (线性融合)")},
            {"gaussian", QStringLiteral("gaussian (高斯融合)")},
        }},
        {"simplify_method", {
            {"douglas_peucker", QStringLiteral("douglas_peucker (道格拉斯-普克)")},
            {"visvalingam", QStringLiteral("visvalingam (维斯瓦林甘)")},
        }},
        {"output_format", {
            {"shp", QStringLiteral("shp (Shapefile)")},
            {"geojson", QStringLiteral("geojson (GeoJSON)")},
            {"gpkg", QStringLiteral("gpkg (GeoPackage)")},
        }},
    };

    auto paramIt = translations.find(paramKey);
    if (paramIt == translations.end()) {
        return QString::fromStdString(rawValue);
    }

    auto valIt = paramIt->second.find(rawValue);
    if (valIt == paramIt->second.end()) {
        return QString::fromStdString(rawValue);
    }

    return valIt->second;
}

QString actionDisplayName(const std::string& pluginName, const std::string& actionKey) {
    auto cfg = getActionUiConfig(pluginName, actionKey);
    if (!cfg.displayName.isEmpty()) return cfg.displayName;
    return QString::fromStdString(actionKey);
}

}
