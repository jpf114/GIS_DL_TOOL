#include <gtest/gtest.h>
#include "core/logger.h"
#include "core/exception.h"
#include "core/platform.h"
#include "core/memory.h"
#include "core/config.h"

using namespace gis_ai;

TEST(CoreTest, LoggerInitialization) {
    EXPECT_NO_THROW(Logger::Instance().Initialize());
    EXPECT_NO_THROW(LOG_INFO("Test info message"));
    EXPECT_NO_THROW(LOG_DEBUG("Test debug message"));
    EXPECT_NO_THROW(LOG_WARN("Test warning message"));
    EXPECT_NO_THROW(LOG_ERROR("Test error message"));
}

TEST(CoreTest, ExceptionBasic) {
    GisAiException ex(ErrorCode::IO, "test error", "test context");
    EXPECT_EQ(ex.GetCode(), ErrorCode::IO);
    EXPECT_EQ(ex.GetContext(), "test context");
    EXPECT_NE(std::string(ex.what()).find("test error"), std::string::npos);
}

TEST(CoreTest, ExceptionTypes) {
    EXPECT_THROW(throw GisAiIOException("io error"), GisAiException);
    EXPECT_THROW(throw GisAiModelException("model error"), GisAiException);
    EXPECT_THROW(throw GisAiAlgorithmException("algo error"), GisAiException);
    EXPECT_THROW(throw GisAiConfigException("config error"), GisAiException);
}

TEST(CoreTest, PlatformPathSeparator) {
    auto sep = Platform::GetPathSeparator();
    EXPECT_FALSE(sep.empty());
}

TEST(CoreTest, PlatformJoinPath) {
    auto result = Platform::JoinPath("path/a", "file.txt");
    EXPECT_NE(result.find("file.txt"), std::string::npos);
    EXPECT_NE(result.find("path"), std::string::npos);
}

TEST(CoreTest, PlatformFileExists) {
    EXPECT_FALSE(Platform::FileExists("/nonexistent/path/file.txt"));
}

TEST(CoreTest, MemoryPoolAllocate) {
    MemoryPool pool(1024);
    void* ptr = pool.Allocate(256);
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(pool.GetTotalAllocated(), 256u);
    pool.Deallocate(ptr, 256);
    EXPECT_EQ(pool.GetTotalAllocated(), 0u);
}

TEST(CoreTest, MemoryPoolReset) {
    MemoryPool pool(1024);
    pool.Allocate(512);
    pool.Allocate(256);
    pool.Reset();
    EXPECT_EQ(pool.GetTotalAllocated(), 0u);
}

TEST(CoreTest, ConfigSetGet) {
    Config::Instance().Set("test_key", std::string("test_value"));
    EXPECT_EQ(Config::Instance().GetString("test_key"), "test_value");
    EXPECT_EQ(Config::Instance().GetString("missing", "default"), "default");
    Config::Instance().Clear();
}

TEST(CoreTest, ConfigTypes) {
    Config::Instance().Set("int_val", 42);
    Config::Instance().Set("double_val", 3.14);
    Config::Instance().Set("bool_val", true);
    EXPECT_EQ(Config::Instance().GetInt("int_val"), 42);
    EXPECT_NEAR(Config::Instance().GetDouble("double_val"), 3.14, 0.001);
    EXPECT_TRUE(Config::Instance().GetBool("bool_val"));
    Config::Instance().Clear();
}
