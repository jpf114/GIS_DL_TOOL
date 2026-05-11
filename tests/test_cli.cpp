#include <gtest/gtest.h>
#include <cstdlib>
#include <string>

class CliIntegrationTest : public ::testing::Test {
protected:
    std::string cli_path_;
    void SetUp() override {
        cli_path_ = std::string(getenv("GIS_AI_CLI_PATH") ? getenv("GIS_AI_CLI_PATH") : "gis_ai_cli");
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
