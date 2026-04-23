#include "core/config.h"
#include "core/logger.h"
#include <iostream>

int main() {
    gis_ai::Logger::Instance().Initialize("demo_config.log");

    auto& config = gis_ai::Config::Instance();

    config.Set("input_dir", std::string("/data/input"));
    config.Set("output_dir", std::string("/data/output"));
    config.Set("tile_size", 512);
    config.Set("overlap", 64);
    config.Set("use_gpu", false);

    std::cout << "input_dir: " << config.GetString("input_dir") << std::endl;
    std::cout << "output_dir: " << config.GetString("output_dir") << std::endl;
    std::cout << "tile_size: " << config.GetInt("tile_size") << std::endl;
    std::cout << "overlap: " << config.GetInt("overlap") << std::endl;
    std::cout << "use_gpu: " << (config.GetBool("use_gpu") ? "true" : "false") << std::endl;

    config.SaveToFile("demo_config_output.json");

    config.LoadFromFile("demo_config_output.json");
    std::cout << "Reloaded tile_size: " << config.GetInt("tile_size") << std::endl;

    return 0;
}
