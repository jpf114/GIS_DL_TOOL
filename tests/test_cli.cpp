#include <gtest/gtest.h>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#endif

class CliIntegrationTest : public ::testing::Test {
protected:
    std::string cli_path_;
    void SetUp() override {
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
