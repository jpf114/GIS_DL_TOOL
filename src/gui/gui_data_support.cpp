#include "gui_data_support.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
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
    if (auto* v = std::get_if<int>(&it->second)) return static_cast<double>(*v);
    return def;
}

std::array<double, 4> getExtentParam(const std::map<std::string, ParamValue>& params, const std::string& key) {
    auto it = params.find(key);
    if (it == params.end()) return {0, 0, 0, 0};
    if (auto* v = std::get_if<std::array<double, 4>>(&it->second)) return *v;
    return {0, 0, 0, 0};
}

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

const std::map<std::string, ParamText>& commonParamTextStorage() {
    static const std::map<std::string, ParamText> kTexts = {
        {"model_path", {QStringLiteral("模型路径"), QStringLiteral("用于推理或分割的 ONNX 模型文件路径。")}},
        {"input_raster", {QStringLiteral("输入影像"), QStringLiteral("待处理的栅格影像文件路径。")}},
        {"input_vector", {QStringLiteral("输入矢量"), QStringLiteral("待处理的矢量数据文件路径。")}},
        {"output_tif", {QStringLiteral("输出栅格"), QStringLiteral("输出栅格结果文件路径。")}},
        {"output_shp", {QStringLiteral("输出矢量"), QStringLiteral("输出矢量结果文件路径。")}},
        {"output_path", {QStringLiteral("输出路径"), QStringLiteral("输出结果文件路径。")}},
        {"output_dir", {QStringLiteral("输出目录"), QStringLiteral("批量结果的输出目录。")}},
        {"input_dir", {QStringLiteral("输入目录"), QStringLiteral("批量处理的输入目录。")}},
        {"tile_size", {QStringLiteral("分块大小"), QStringLiteral("大图分块推理时使用的切片大小。")}},
        {"stride", {QStringLiteral("步长"), QStringLiteral("滑窗推理时相邻窗口的移动步长。")}},
        {"blend_mode", {QStringLiteral("融合方式"), QStringLiteral("分块重叠区域的融合方式。")}},
        {"target_class", {QStringLiteral("目标类别"), QStringLiteral("分割或推理时关注的目标类别编号。")}},
        {"skip_nodata", {QStringLiteral("跳过无数据"), QStringLiteral("推理时是否跳过无数据区域。")}},
        {"simplify_tolerance", {QStringLiteral("简化容差"), QStringLiteral("矢量简化时使用的容差值。")}},
        {"buffer_distance", {QStringLiteral("缓冲距离"), QStringLiteral("矢量缓冲区分析的距离值。")}},
        {"threshold_value", {QStringLiteral("阈值"), QStringLiteral("阈值分割时使用的阈值。")}},
        {"resample_method", {QStringLiteral("重采样方式"), QStringLiteral("栅格重采样时使用的方法。")}},
        {"mosaic_strategy", {QStringLiteral("镶嵌策略"), QStringLiteral("栅格镶嵌合并时使用的策略。")}},
        {"normalize_mode", {QStringLiteral("归一化方式"), QStringLiteral("影像归一化处理时使用的方式。")}},
        {"clip_extent", {QStringLiteral("裁剪范围"), QStringLiteral("裁剪范围，格式为 Xmin、Ymin、Xmax、Ymax。")}},
    };
    return kTexts;
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
        specs.push_back({"target_class", "目标类别", "推理目标类别编号", ParamType::Int, false, int{1}, int{0}, int{255}, {}});
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
            cfg.visibleKeys = {"input_dir", "model_path", "output_dir", "target_class"};
            cfg.requiredKeys = {"input_dir", "model_path", "output_dir"};
        }
    }

    return cfg;
}

bool isKnownAction(const std::string& pluginName, const std::string& actionKey) {
    const auto cfg = getActionUiConfig(pluginName, actionKey);
    return !cfg.displayName.isEmpty()
        || !cfg.description.isEmpty()
        || !cfg.visibleKeys.empty()
        || !cfg.requiredKeys.empty();
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

    if (pluginName == "batch" && actionKey == "batch_inference") {
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
        if (paramKey == "output_shp") {
            config.suggestedSuffix = ".shp";
            config.placeholder = QString::fromUtf8("请选择输出文件，建议使用 .shp、.gpkg 或 .geojson");
            config.saveFilter = QStringLiteral("Shapefile 文件 (*.shp);;GeoPackage 文件 (*.gpkg);;GeoJSON 文件 (*.geojson);;所有文件 (*)");
            return config;
        }

        config.suggestedSuffix = defaultSuffixForOutput(pluginName, action, paramKey, ".tif");

        if (config.suggestedSuffix == ".tif" || config.suggestedSuffix == ".tiff") {
            config.placeholder = QString::fromUtf8("请选择输出文件，建议使用 .tif");
            config.saveFilter = QStringLiteral("GeoTIFF 文件 (*.tif *.tiff);;所有文件 (*)");
        } else if (config.suggestedSuffix == ".shp") {
            config.placeholder = QString::fromUtf8("请选择输出文件，建议使用 .shp、.gpkg 或 .geojson");
            config.saveFilter = QStringLiteral("Shapefile 文件 (*.shp);;GeoPackage 文件 (*.gpkg);;GeoJSON 文件 (*.geojson);;所有文件 (*)");
        } else if (config.suggestedSuffix == ".gpkg") {
            config.placeholder = QString::fromUtf8("请选择输出文件，建议使用 .gpkg");
            config.saveFilter = QStringLiteral("GeoPackage 文件 (*.gpkg);;所有文件 (*)");
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
        config.openFilter = QStringLiteral("ONNX 模型文件 (*.onnx);;所有文件 (*)");
        return config;
    }

    if (paramKey.find("raster") != std::string::npos || paramKey == "input_raster") {
        config.placeholder = QString::fromUtf8("请选择栅格文件，例如 .tif、.img、.vrt");
        config.openFilter = QStringLiteral("栅格文件 (*.tif *.tiff *.img *.vrt *.png *.jpg *.jpeg *.bmp);;GeoTIFF 文件 (*.tif *.tiff);;所有文件 (*)");
        return config;
    }

    if (paramKey.find("vector") != std::string::npos || paramKey == "input_vector") {
        config.placeholder = QString::fromUtf8("请选择矢量文件，例如 .gpkg、.shp、.geojson");
        config.openFilter = QStringLiteral("矢量文件 (*.gpkg *.shp *.geojson *.json *.kml *.csv);;GeoPackage 文件 (*.gpkg);;Shapefile 文件 (*.shp);;GeoJSON 文件 (*.geojson *.json);;所有文件 (*)");
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
            {"basic", QStringLiteral("basic（基础）")},
            {"otsu", QStringLiteral("otsu（大津法）")},
            {"adaptive", QStringLiteral("adaptive（自适应）")},
        }},
        {"match_method", {
            {"exact", QStringLiteral("exact（精确匹配）")},
            {"partial", QStringLiteral("partial（部分匹配）")},
            {"fuzzy", QStringLiteral("fuzzy（模糊匹配）")},
        }},
        {"transform", {
            {"identity", QStringLiteral("identity（无变换）")},
            {"normalize", QStringLiteral("normalize（归一化）")},
            {"standardize", QStringLiteral("standardize（标准化）")},
        }},
        {"resample", {
            {"nearest", QStringLiteral("nearest（最近邻）")},
            {"bilinear", QStringLiteral("bilinear（双线性）")},
            {"cubic", QStringLiteral("cubic（三次卷积）")},
            {"lanczos", QStringLiteral("lanczos（兰佐斯）")},
            {"average", QStringLiteral("average（平均值）")},
            {"mode", QStringLiteral("mode（众数）")},
        }},
        {"blend", {
            {"none", QStringLiteral("none（无融合）")},
            {"linear", QStringLiteral("linear（线性融合）")},
            {"gaussian", QStringLiteral("gaussian（高斯融合）")},
        }},
        {"simplify_method", {
            {"douglas_peucker", QStringLiteral("douglas_peucker（道格拉斯-普克）")},
            {"visvalingam", QStringLiteral("visvalingam（维斯瓦林甘）")},
        }},
        {"output_format", {
            {"shp", QStringLiteral("shp（Shapefile）")},
            {"geojson", QStringLiteral("geojson（GeoJSON）")},
            {"gpkg", QStringLiteral("gpkg（GeoPackage）")},
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
