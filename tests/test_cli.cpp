#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#endif

class CliIntegrationTest : public ::testing::Test {
protected:
    const std::filesystem::path test_output_dir_ = "test_output";
    std::string cli_path_;

    void SetUp() override {
        std::filesystem::create_directories(test_output_dir_);
        const char* env_path = getenv("GIS_AI_CLI_PATH");
        if (env_path && env_path[0] != '\0') {
            cli_path_ = std::string("\"") + env_path + "\"";
#ifdef _WIN32
            std::string dir = env_path;
            auto pos = dir.find_last_of("\\/");
            if (pos != std::string::npos) {
                dir = dir.substr(0, pos);
            }
            std::string current_path;
            DWORD len = GetEnvironmentVariableA("PATH", nullptr, 0);
            if (len > 0) {
                current_path.resize(len - 1);
                GetEnvironmentVariableA("PATH", &current_path[0], len);
            }
            std::string new_path = dir + ";" + current_path;
            SetEnvironmentVariableA("PATH", new_path.c_str());
#endif
        } else {
            cli_path_ = "gis_ai_cli";
        }
    }

    int runCli(const std::string& args) {
        return system((cli_path_ + " " + args).c_str());
    }

    static void writeFile(const std::filesystem::path& path, const std::string& content) {
        std::ofstream out(path, std::ios::binary);
        ASSERT_TRUE(out.is_open()) << "Failed to open " << path.string();
        out << content;
    }
};

TEST_F(CliIntegrationTest, VersionCommand) {
    EXPECT_EQ(runCli("version"), 0);
}

TEST_F(CliIntegrationTest, HelpCommand) {
    EXPECT_EQ(runCli("help"), 0);
}

TEST_F(CliIntegrationTest, UnknownCommand) {
    EXPECT_NE(runCli("unknown_command_xyz"), 0);
}

TEST_F(CliIntegrationTest, SegmentReturnsModelErrorCodeForMissingModel) {
    EXPECT_EQ(runCli("segment test_data/models/missing_model.onnx test_data/raster/test_100x100.tif"),
              2001);
}

TEST_F(CliIntegrationTest, InferenceReturnsModelErrorCodeForMissingModel) {
    EXPECT_EQ(runCli("inference test_data/models/missing_model.onnx test_data/raster/test_100x100.tif"),
              2001);
}

TEST_F(CliIntegrationTest, BatchReturnsModelErrorCodeForMissingModel) {
    EXPECT_EQ(runCli("batch test_data/models/missing_model.onnx test_data/batch_input test_output/batch_cli_probe"),
              2001);
}

TEST_F(CliIntegrationTest, BatchInferenceReturnsModelErrorCodeForMissingModel) {
    EXPECT_EQ(runCli("batch-inference test_data/models/missing_model.onnx test_data/batch_input test_output/batch_infer_cli_probe"),
              2001);
}

TEST_F(CliIntegrationTest, RunReturnsConfigErrorCodeForMissingConfigFile) {
    EXPECT_EQ(runCli("run not_exists.json"), 4001);
}

TEST_F(CliIntegrationTest, RunReturnsConfigErrorCodeForInvalidJson) {
    const auto config_path = test_output_dir_ / "bad_config.json";
    writeFile(config_path, "{bad json}");

    EXPECT_EQ(runCli("run test_output/bad_config.json"), 4001);
}

TEST_F(CliIntegrationTest, RunRejectsUnknownTaskType) {
    const auto config_path = test_output_dir_ / "unknown_task_config.json";
    writeFile(config_path,
              R"({"task_type":"not_real","model_path":"test_data/models/test_seg_model.onnx","input_path":"test_data/raster/test_100x100.tif"})");

    EXPECT_EQ(runCli("run test_output/unknown_task_config.json"), 4001);
}

TEST_F(CliIntegrationTest, RunSupportsBatchInferenceTaskType) {
    const auto config_path = test_output_dir_ / "batch_inference_missing_model.json";
    writeFile(config_path,
              R"({"task_type":"batch_inference","model_path":"test_data/models/missing_model.onnx","input_dir":"test_data/batch_input","output_dir":"test_output/batch_infer_from_config"})");

    EXPECT_EQ(runCli("run test_output/batch_inference_missing_model.json"), 2001);
}

TEST_F(CliIntegrationTest, RunWritesJsonReportWhenRequested) {
    const auto config_path = test_output_dir_ / "report_missing_model_config.json";
    const auto report_path = test_output_dir_ / "cli_report_test.json";
    writeFile(config_path,
              R"({"task_type":"segment","model_path":"test_data/models/missing_model.onnx","input_path":"test_data/raster/test_100x100.tif"})");
    std::filesystem::remove(report_path);

    EXPECT_EQ(runCli("run test_output/report_missing_model_config.json --report test_output/cli_report_test.json"),
              2001);

    ASSERT_TRUE(std::filesystem::exists(report_path));
    std::ifstream report(report_path);
    ASSERT_TRUE(report.is_open());
    const std::string content((std::istreambuf_iterator<char>(report)),
                              std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("\"success\""), std::string::npos);
    EXPECT_NE(content.find("\"error_code\""), std::string::npos);
    EXPECT_NE(content.find("\"output_files\""), std::string::npos);
}
