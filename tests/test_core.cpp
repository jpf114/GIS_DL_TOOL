#include <gtest/gtest.h>
#include "core/logger.h"
#include "core/exception.h"
#include "core/platform.h"
#include "core/memory.h"
#include "core/config.h"

#include <fstream>
#include <filesystem>

using namespace gis_ai;

class CoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config::Instance().Clear();
    }
    void TearDown() override {
        Config::Instance().Clear();
    }
};

TEST_F(CoreTest, LoggerInitialization) {
    EXPECT_NO_THROW(Logger::Instance().Initialize());
    EXPECT_NO_THROW(LOG_INFO("Test info message"));
    EXPECT_NO_THROW(LOG_DEBUG("Test debug message"));
    EXPECT_NO_THROW(LOG_WARN("Test warning message"));
    EXPECT_NO_THROW(LOG_ERROR("Test error message"));
}

TEST_F(CoreTest, LoggerAutoInit) {
    Logger::Instance().EnsureInitialized();
    EXPECT_NO_THROW(LOG_INFO("Auto-init test"));
}

TEST_F(CoreTest, ExceptionBasic) {
    GisAiException ex(ErrorCode::IO, "test error", "test context");
    EXPECT_EQ(ex.GetCode(), ErrorCode::IO);
    EXPECT_EQ(ex.GetContext(), "test context");
    EXPECT_NE(std::string(ex.what()).find("test error"), std::string::npos);
}

TEST_F(CoreTest, ExceptionTypes) {
    EXPECT_THROW(throw GisAiIOException("io error"), GisAiException);
    EXPECT_THROW(throw GisAiModelException("model error"), GisAiException);
    EXPECT_THROW(throw GisAiAlgorithmException("algo error"), GisAiException);
    EXPECT_THROW(throw GisAiConfigException("config error"), GisAiException);
}

TEST_F(CoreTest, PlatformPathSeparator) {
    auto sep = Platform::GetPathSeparator();
    EXPECT_FALSE(sep.empty());
}

TEST_F(CoreTest, PlatformJoinPath) {
    auto result = Platform::JoinPath("path/a", "file.txt");
    EXPECT_NE(result.find("file.txt"), std::string::npos);
    EXPECT_NE(result.find("path"), std::string::npos);
}

TEST_F(CoreTest, PlatformFileExists) {
    EXPECT_FALSE(Platform::FileExists("/nonexistent/path/file.txt"));
}

TEST_F(CoreTest, MemoryPoolAllocate) {
    MemoryPool pool(1024);
    void* ptr = pool.Allocate(256);
    EXPECT_NE(ptr, nullptr);
    EXPECT_GE(pool.GetTotalAllocated(), 256u);
    pool.Deallocate(ptr, 256);
    EXPECT_EQ(pool.GetTotalAllocated(), 0u);
}

TEST_F(CoreTest, MemoryPoolReset) {
    MemoryPool pool(1024);
    pool.Allocate(512);
    pool.Allocate(256);
    pool.Reset();
    EXPECT_EQ(pool.GetTotalAllocated(), 0u);
}

TEST_F(CoreTest, ConfigSetGet) {
    Config::Instance().Set("test_key", std::string("test_value"));
    EXPECT_EQ(Config::Instance().GetString("test_key"), "test_value");
    EXPECT_EQ(Config::Instance().GetString("missing", "default"), "default");
}

TEST_F(CoreTest, ConfigTypes) {
    Config::Instance().Set("int_val", 42);
    Config::Instance().Set("double_val", 3.14);
    Config::Instance().Set("bool_val", true);
    EXPECT_EQ(Config::Instance().GetInt("int_val"), 42);
    EXPECT_NEAR(Config::Instance().GetDouble("double_val"), 3.14, 0.001);
    EXPECT_TRUE(Config::Instance().GetBool("bool_val"));
}

TEST_F(CoreTest, ConfigLoadFromValidJson) {
    std::string json_content = R"({"name": "test", "count": 10, "ratio": 0.5, "enabled": true})";
    EXPECT_TRUE(Config::Instance().LoadFromString(json_content));
    EXPECT_EQ(Config::Instance().GetString("name"), "test");
    EXPECT_EQ(Config::Instance().GetInt("count"), 10);
    EXPECT_NEAR(Config::Instance().GetDouble("ratio"), 0.5, 0.001);
    EXPECT_TRUE(Config::Instance().GetBool("enabled"));
}

TEST_F(CoreTest, ConfigLoadFromInvalidJson) {
    EXPECT_FALSE(Config::Instance().LoadFromString("{invalid json}"));
    EXPECT_FALSE(Config::Instance().LoadFromString(""));
}

TEST_F(CoreTest, ConfigLoadFromNestedJson) {
    std::string json_content = R"({"top": "value", "nested": {"inner": 42}})";
    EXPECT_TRUE(Config::Instance().LoadFromString(json_content));
    EXPECT_EQ(Config::Instance().GetString("top"), "value");
}

TEST_F(CoreTest, ConfigLoadFromArrayJson) {
    std::string json_content = R"({"items": [1, 2, 3], "name": "array_test"})";
    EXPECT_TRUE(Config::Instance().LoadFromString(json_content));
    EXPECT_EQ(Config::Instance().GetString("name"), "array_test");
}

TEST_F(CoreTest, ConfigSaveAndLoad) {
    std::string temp_path = std::filesystem::temp_directory_path().string() + "/gis_ai_test_config.json";
    Config::Instance().Set("save_key", std::string("save_value"));
    Config::Instance().Set("save_int", 99);
    Config::Instance().Set("save_bool", false);

    EXPECT_NO_THROW(Config::Instance().SaveToFile(temp_path));
    EXPECT_TRUE(std::filesystem::exists(temp_path));

    Config::Instance().Clear();
    EXPECT_TRUE(Config::Instance().LoadFromFile(temp_path));
    EXPECT_EQ(Config::Instance().GetString("save_key"), "save_value");
    EXPECT_EQ(Config::Instance().GetInt("save_int"), 99);
    EXPECT_FALSE(Config::Instance().GetBool("save_bool"));

    std::filesystem::remove(temp_path);
}

TEST_F(CoreTest, ConfigSpecialCharacters) {
    std::string json_content = R"({"path": "C:\\Users\\test", "msg": "hello \"world\""})";
    EXPECT_TRUE(Config::Instance().LoadFromString(json_content));
}

TEST_F(CoreTest, ConfigHasRemove) {
    Config::Instance().Set("temp_key", 1);
    EXPECT_TRUE(Config::Instance().Has("temp_key"));
    Config::Instance().Remove("temp_key");
    EXPECT_FALSE(Config::Instance().Has("temp_key"));
}

TEST_F(CoreTest, ConfigLoadNonexistentFile) {
    EXPECT_FALSE(Config::Instance().LoadFromFile("/nonexistent/config.json"));
}
