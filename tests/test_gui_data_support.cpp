#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <gdal_priv.h>

#include "gui_data_support.h"
#include "data_inspector.h"
#include "action_dispatcher.h"
#include "param_card_widget.h"
#include "qt_progress_reporter.h"
#include "io/vector_io.h"

using namespace gis_ai::gui;

namespace {

gis_ai::VectorData MakePolygon(double minX, double minY, double maxX, double maxY) {
    gis_ai::VectorData data;
    data.feature_type = gis_ai::FeatureType::Polygon;
    data.projection = "EPSG:4326";

    gis_ai::Feature feature;
    feature.type = gis_ai::FeatureType::Polygon;
    feature.coordinates = {
        {minX, minY, 0.0},
        {maxX, minY, 0.0},
        {maxX, maxY, 0.0},
        {minX, maxY, 0.0},
        {minX, minY, 0.0},
    };
    data.features.push_back(feature);
    return data;
}

}

TEST(GuiDataSupportTest, DetectDataKindRaster) {
    EXPECT_EQ(detectDataKind("test.tif"), DataKind::Raster);
    EXPECT_EQ(detectDataKind("test.tiff"), DataKind::Raster);
    EXPECT_EQ(detectDataKind("test.img"), DataKind::Raster);
    EXPECT_EQ(detectDataKind("test.TIF"), DataKind::Raster);
    EXPECT_EQ(detectDataKind("test.Tiff"), DataKind::Raster);
}

TEST(GuiDataSupportTest, DetectDataKindVector) {
    EXPECT_EQ(detectDataKind("test.shp"), DataKind::Vector);
    EXPECT_EQ(detectDataKind("test.gpkg"), DataKind::Vector);
    EXPECT_EQ(detectDataKind("test.geojson"), DataKind::Vector);
    EXPECT_EQ(detectDataKind("test.SHP"), DataKind::Vector);
}

TEST(GuiDataSupportTest, DetectDataKindUnknown) {
    EXPECT_EQ(detectDataKind("test.doc"), DataKind::Unknown);
    EXPECT_EQ(detectDataKind("test.pdf"), DataKind::Unknown);
    EXPECT_EQ(detectDataKind("test"), DataKind::Unknown);
    EXPECT_EQ(detectDataKind(""), DataKind::Unknown);
}

TEST(GuiDataSupportTest, IsSupportedDataPath) {
    EXPECT_TRUE(isSupportedDataPath("test.tif"));
    EXPECT_TRUE(isSupportedDataPath("test.gpkg"));
    EXPECT_FALSE(isSupportedDataPath("test.doc"));
}

TEST(GuiDataSupportTest, DataKindDisplayName) {
    EXPECT_EQ(dataKindDisplayName(DataKind::Raster), "栅格");
    EXPECT_EQ(dataKindDisplayName(DataKind::Vector), "矢量");
    EXPECT_EQ(dataKindDisplayName(DataKind::Unknown), "未知");
}

TEST(GuiDataSupportTest, BuildSuggestedOutputPathSegment) {
    std::string result = buildSuggestedOutputPath(
        "D:/data/input.tif", "segment", "segment_raster", "output_tif");
    EXPECT_NE(result.find("segment"), std::string::npos);
    EXPECT_NE(result.find(".tif"), std::string::npos);
}

TEST(GuiDataSupportTest, BuildSuggestedOutputPathEmpty) {
    std::string result = buildSuggestedOutputPath("", "segment", "segment_raster", "output_tif");
    EXPECT_TRUE(result.empty());
}

TEST(GuiDataSupportTest, ComputeDerivedOutputUpdateEmpty) {
    DerivedOutputUpdate update = computeDerivedOutputUpdate(
        "", "", "D:/data/input.tif", "segment", "segment_raster", "output_tif");
    EXPECT_TRUE(update.shouldApply);
    EXPECT_FALSE(update.value.empty());
}

TEST(GuiDataSupportTest, ComputeDerivedOutputUpdateUserEdited) {
    DerivedOutputUpdate update = computeDerivedOutputUpdate(
        "D:/custom/output.tif", "", "D:/data/input.tif", "segment", "segment_raster", "output_tif");
    EXPECT_FALSE(update.shouldApply);
}

TEST(GuiDataSupportTest, ComputeDerivedOutputUpdateAutoTracking) {
    DerivedOutputUpdate first = computeDerivedOutputUpdate(
        "", "", "D:/data/input.tif", "segment", "segment_raster", "output_tif");
    EXPECT_TRUE(first.shouldApply);

    DerivedOutputUpdate second = computeDerivedOutputUpdate(
        first.value, first.autoValue, "D:/data/other.tif", "segment", "segment_raster", "output_tif");
    EXPECT_TRUE(second.shouldApply);
}

TEST(GuiDataSupportTest, ShouldAutoFillLayerValue) {
    EXPECT_TRUE(shouldAutoFillLayerValue("", "", "layer1"));
    EXPECT_FALSE(shouldAutoFillLayerValue("existing", "", "layer1"));
    EXPECT_TRUE(shouldAutoFillLayerValue("old_layer", "old_layer", "new_layer"));
    EXPECT_FALSE(shouldAutoFillLayerValue("user_layer", "auto_layer", "new_layer"));
}

TEST(GuiDataSupportTest, ShouldAutoFillExtentValue) {
    EXPECT_TRUE(shouldAutoFillExtentValue(std::nullopt, std::nullopt, true));
    EXPECT_FALSE(shouldAutoFillExtentValue(std::nullopt, std::nullopt, false));
    std::array<double, 4> zeroExtent = {0, 0, 0, 0};
    EXPECT_TRUE(shouldAutoFillExtentValue(zeroExtent, std::nullopt, true));
    std::array<double, 4> validExtent = {1, 2, 3, 4};
    EXPECT_FALSE(shouldAutoFillExtentValue(validExtent, std::nullopt, true));
    EXPECT_TRUE(shouldAutoFillExtentValue(validExtent, validExtent, true));
}

TEST(GuiDataSupportTest, ValidateActionSegmentTileSize) {
    std::map<std::string, ParamValue> params;
    params["tile_size"] = 32;
    auto issue = validateActionSpecificParams("segment", "segment_raster", params);
    ASSERT_TRUE(issue.has_value());
    EXPECT_EQ(issue->key, "tile_size");
}

TEST(GuiDataSupportTest, ValidateActionSegmentValid) {
    std::map<std::string, ParamValue> params;
    params["tile_size"] = 256;
    auto issue = validateActionSpecificParams("segment", "segment_raster", params);
    EXPECT_FALSE(issue.has_value());
}

TEST(GuiDataSupportTest, ValidateActionSegmentStride) {
    std::map<std::string, ParamValue> params;
    params["tile_size"] = 256;
    params["stride"] = 512;
    auto issue = validateActionSpecificParams("segment", "segment_raster", params);
    ASSERT_TRUE(issue.has_value());
    EXPECT_EQ(issue->key, "stride");
}

TEST(GuiDataSupportTest, ValidateActionSegmentOutputFormat) {
    std::map<std::string, ParamValue> params;
    params["output_tif"] = std::string("result.doc");
    auto issue = validateActionSpecificParams("segment", "segment_raster", params);
    ASSERT_TRUE(issue.has_value());
    EXPECT_EQ(issue->key, "output_tif");
}

TEST(GuiDataSupportTest, ValidateActionVectorBuffer) {
    std::map<std::string, ParamValue> params;
    params["buffer_distance"] = -1.0;
    auto issue = validateActionSpecificParams("vector", "vector_buffer", params);
    ASSERT_TRUE(issue.has_value());
    EXPECT_EQ(issue->key, "buffer_distance");
}

TEST(GuiDataSupportTest, ValidateBatchInferenceTargetClassRange) {
    std::map<std::string, ParamValue> params;
    params["target_class"] = 999;
    auto issue = validateActionSpecificParams("batch", "batch_inference", params);
    ASSERT_TRUE(issue.has_value());
    EXPECT_EQ(issue->key, "target_class");
}

TEST(GuiDataSupportTest, ValidateBatchInferenceIgnoresSegmentationTilingParams) {
    std::map<std::string, ParamValue> params;
    params["tile_size"] = 32;
    params["stride"] = 999;
    params["target_class"] = 1;
    auto issue = validateActionSpecificParams("batch", "batch_inference", params);
    EXPECT_FALSE(issue.has_value());
}

TEST(GuiDataSupportTest, BuildExecuteButtonStateNoSelection) {
    auto state = buildExecuteButtonState(false, {});
    EXPECT_FALSE(state.enabled);
    EXPECT_EQ(state.badgeText, QStringLiteral("就绪"));
}

TEST(GuiDataSupportTest, BuildExecuteButtonStateValidationError) {
    auto state = buildExecuteButtonState(true, QStringLiteral("参数错误"));
    EXPECT_FALSE(state.enabled);
    EXPECT_EQ(state.badgeText, QStringLiteral("待修正"));
}

TEST(GuiDataSupportTest, BuildExecuteButtonStateReady) {
    auto state = buildExecuteButtonState(true, {});
    EXPECT_TRUE(state.enabled);
    EXPECT_EQ(state.badgeText, QStringLiteral("可执行"));
}

TEST(GuiDataSupportTest, ResolveHighlightedParamKeyNoSelection) {
    std::vector<ParamSpec> specs;
    std::map<std::string, ParamValue> params;
    auto result = resolveHighlightedParamKey(false, specs, params, std::nullopt);
    EXPECT_TRUE(result.empty());
}

TEST(GuiDataSupportTest, ResolveHighlightedParamKeyActionIssue) {
    std::vector<ParamSpec> specs;
    std::map<std::string, ParamValue> params;
    auto issue = ActionValidationIssue{"tile_size", QStringLiteral("错误")};
    auto result = resolveHighlightedParamKey(true, specs, params, issue);
    EXPECT_EQ(result, "tile_size");
}

TEST(GuiDataSupportTest, LocalizeResultMessageKnown) {
    EXPECT_EQ(localizeResultMessage("Segmentation completed successfully"), "分割完成");
    EXPECT_EQ(localizeResultMessage("Cancelled"), "操作已取消");
}

TEST(GuiDataSupportTest, LocalizeResultMessageUnknown) {
    EXPECT_EQ(localizeResultMessage("Some unknown error"), "Some unknown error");
}

TEST(GuiDataSupportTest, LocalizeResultMessageFileError) {
    std::string result = localizeResultMessage("Cannot open file: test.tif");
    EXPECT_NE(result.find("无法打开文件"), std::string::npos);
}

TEST(GuiDataSupportTest, BuildFileParamUiConfigModel) {
    auto config = buildFileParamUiConfig("segment", "segment_raster", "model_path", ParamType::FilePath);
    EXPECT_FALSE(config.isOutput);
    EXPECT_TRUE(config.openFilter.contains(QStringLiteral("onnx")));
}

TEST(GuiDataSupportTest, BuildFileParamUiConfigOutputTif) {
    auto config = buildFileParamUiConfig("segment", "segment_raster", "output_tif", ParamType::FilePath);
    EXPECT_TRUE(config.isOutput);
    EXPECT_EQ(config.suggestedSuffix, ".tif");
}

TEST(GuiDataSupportTest, BuildFileParamUiConfigCrs) {
    auto config = buildFileParamUiConfig("segment", "segment_raster", "dst_srs", ParamType::CRS);
    EXPECT_TRUE(config.placeholder.contains(QStringLiteral("EPSG")));
}

TEST(GuiDataSupportTest, FindCommonParamText) {
    const ParamText* text = findCommonParamText("model_path");
    ASSERT_NE(text, nullptr);
    EXPECT_FALSE(text->displayName.isEmpty());
}

TEST(GuiDataSupportTest, FindCommonParamTextUnknown) {
    const ParamText* text = findCommonParamText("nonexistent_key");
    EXPECT_EQ(text, nullptr);
}

TEST(GuiDataSupportTest, ActionDisplayName) {
    QString name = actionDisplayName("segment", "segment_raster");
    EXPECT_FALSE(name.isEmpty());
}

TEST(GuiDataSupportTest, InferencePluginDoesNotExposeBatchActionConfig) {
    const auto cfg = getActionUiConfig("inference", "inference_batch");
    EXPECT_TRUE(cfg.visibleKeys.empty());
    EXPECT_TRUE(cfg.requiredKeys.empty());
}

TEST(GuiDataSupportTest, BatchInferenceUsesDirectoryInputsAndOutputs) {
    const auto cfg = getActionUiConfig("batch", "batch_inference");
    EXPECT_TRUE(cfg.visibleKeys.count("input_dir") > 0);
    EXPECT_TRUE(cfg.visibleKeys.count("output_dir") > 0);
    EXPECT_TRUE(cfg.requiredKeys.count("input_dir") > 0);
    EXPECT_TRUE(cfg.requiredKeys.count("output_dir") > 0);
    EXPECT_EQ(cfg.visibleKeys.count("input_raster"), 0);
    EXPECT_EQ(cfg.visibleKeys.count("output_path"), 0);
}

TEST(GuiDataSupportTest, BatchInferenceExposesTargetClassInsteadOfSegmentationControls) {
    const auto cfg = getActionUiConfig("batch", "batch_inference");
    EXPECT_TRUE(cfg.visibleKeys.count("target_class") > 0);
    EXPECT_EQ(cfg.visibleKeys.count("tile_size"), 0);
    EXPECT_EQ(cfg.visibleKeys.count("stride"), 0);
    EXPECT_EQ(cfg.visibleKeys.count("blend_mode"), 0);

    const auto specs = buildEffectiveParamSpecs("batch", "batch_inference");
    bool foundTargetClass = false;
    for (const auto& spec : specs) {
        if (spec.key == "target_class") {
            foundTargetClass = true;
            break;
        }
    }
    EXPECT_TRUE(foundTargetClass);
}

TEST(GuiDataSupportTest, ExecuteActionVectorClipUsesClipExtent) {
    GDALAllRegister();

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    const QString inputPath = tempDir.filePath("input.gpkg");
    const QString outputPath = tempDir.filePath("output.gpkg");

    gis_ai::VectorIO vectorIo;
    vectorIo.Save(MakePolygon(0.0, 0.0, 10.0, 10.0), inputPath.toStdString());

    std::map<std::string, ParamValue> params;
    params["input_vector"] = inputPath.toStdString();
    params["output_path"] = outputPath.toStdString();
    params["clip_extent"] = std::array<double, 4>{2.0, 2.0, 4.0, 4.0};

    QtProgressReporter reporter(QStringLiteral("vector_clip_test"));
    ASSERT_TRUE(executeAction("vector", "vector_clip", params, reporter));

    auto result = vectorIo.Load(outputPath.toStdString());
    ASSERT_NE(result, nullptr);
    ASSERT_FALSE(result->features.empty());

    for (const auto& feature : result->features) {
        for (const auto& coord : feature.coordinates) {
            EXPECT_GE(coord.x, 2.0);
            EXPECT_LE(coord.x, 4.0);
            EXPECT_GE(coord.y, 2.0);
            EXPECT_LE(coord.y, 4.0);
        }
    }
}
