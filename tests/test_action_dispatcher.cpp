#include <gtest/gtest.h>

#include "gui/action_dispatcher.h"
#include "gui/param_utils.h"
#include "gui/qt_progress_reporter.h"

#include <map>
#include <string>

using namespace gis_ai::gui;

class LocalizeResultMessageTest : public ::testing::Test {};

TEST_F(LocalizeResultMessageTest, KnownMessagesAreTranslated) {
    EXPECT_EQ(localizeResultMessage("Segmentation completed successfully"),
              std::string("分割完成"));
    EXPECT_EQ(localizeResultMessage("Batch segmentation completed"),
              std::string("批量分割完成"));
    EXPECT_EQ(localizeResultMessage("Inference completed successfully"),
              std::string("推理完成"));
    EXPECT_EQ(localizeResultMessage("Cancelled"),
              std::string("操作已取消"));
}

TEST_F(LocalizeResultMessageTest, UnknownMessageReturnedAsIs) {
    EXPECT_EQ(localizeResultMessage("some unknown message"),
              std::string("some unknown message"));
}

TEST_F(LocalizeResultMessageTest, EmptyMessageReturnsEmpty) {
    EXPECT_EQ(localizeResultMessage(""), std::string(""));
}

class ParamUtilsTest : public ::testing::Test {
protected:
    std::map<std::string, ParamValue> params_{
        {"str_key", std::string("hello")},
        {"int_key", int{42}},
        {"dbl_key", double{3.14}},
        {"bool_key", true},
        {"extent_key", std::array<double, 4>{1.0, 2.0, 3.0, 4.0}},
    };
};

TEST_F(ParamUtilsTest, GetStringParamExisting) {
    EXPECT_EQ(getStringParam(params_, "str_key"), "hello");
}

TEST_F(ParamUtilsTest, GetStringParamMissing) {
    EXPECT_EQ(getStringParam(params_, "nonexistent"), "");
}

TEST_F(ParamUtilsTest, GetStringParamWrongType) {
    EXPECT_EQ(getStringParam(params_, "int_key"), "");
}

TEST_F(ParamUtilsTest, GetIntParamExisting) {
    EXPECT_EQ(getIntParam(params_, "int_key"), 42);
}

TEST_F(ParamUtilsTest, GetIntParamMissing) {
    EXPECT_EQ(getIntParam(params_, "nonexistent", -1), -1);
}

TEST_F(ParamUtilsTest, GetDoubleParamExisting) {
    EXPECT_DOUBLE_EQ(getDoubleParam(params_, "dbl_key"), 3.14);
}

TEST_F(ParamUtilsTest, GetDoubleParamFromInt) {
    EXPECT_DOUBLE_EQ(getDoubleParam(params_, "int_key"), 42.0);
}

TEST_F(ParamUtilsTest, GetDoubleParamMissing) {
    EXPECT_DOUBLE_EQ(getDoubleParam(params_, "nonexistent", -1.0), -1.0);
}

TEST_F(ParamUtilsTest, GetBoolParamExisting) {
    EXPECT_TRUE(getBoolParam(params_, "bool_key"));
}

TEST_F(ParamUtilsTest, GetBoolParamMissing) {
    EXPECT_FALSE(getBoolParam(params_, "nonexistent"));
}

TEST_F(ParamUtilsTest, GetExtentParamExisting) {
    auto ext = getExtentParam(params_, "extent_key");
    EXPECT_DOUBLE_EQ(ext[0], 1.0);
    EXPECT_DOUBLE_EQ(ext[1], 2.0);
    EXPECT_DOUBLE_EQ(ext[2], 3.0);
    EXPECT_DOUBLE_EQ(ext[3], 4.0);
}

TEST_F(ParamUtilsTest, GetExtentParamMissing) {
    auto ext = getExtentParam(params_, "nonexistent");
    EXPECT_DOUBLE_EQ(ext[0], 0.0);
    EXPECT_DOUBLE_EQ(ext[3], 0.0);
}

class ExecuteActionTest : public ::testing::Test {
protected:
    QtProgressReporter reporter_{QStringLiteral("test-task")};
};

TEST_F(ExecuteActionTest, UnknownPluginReturnsFalse) {
    std::map<std::string, ParamValue> params;
    EXPECT_FALSE(executeAction("nonexistent_plugin", "some_action", params, reporter_));
}

TEST_F(ExecuteActionTest, UnknownActionReturnsFalse) {
    std::map<std::string, ParamValue> params;
    EXPECT_FALSE(executeAction("segment", "nonexistent_action", params, reporter_));
}

TEST_F(ExecuteActionTest, MissingInputFileReturnsFalse) {
    std::map<std::string, ParamValue> params;
    params["input_raster"] = std::string("/nonexistent/path/file.tif");
    params["output_path"] = std::string("/tmp/output.tif");
    EXPECT_FALSE(executeAction("raster", "raster_threshold", params, reporter_));
}

TEST_F(ExecuteActionTest, EmptyParamsReturnsFalse) {
    std::map<std::string, ParamValue> params;
    EXPECT_FALSE(executeAction("segment", "segment_full", params, reporter_));
}
