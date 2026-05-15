#include "fusion/task_config.h"
#include "core/logger.h"

#include <iostream>
#include <string>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

static void PrintUsage() {
    std::cout << "GIS AI 算法库 - 命令行工具 v0.1.0\n\n"
              << "用法:\n"
              << "  gis_ai_cli <command> [options]\n\n"
              << "命令:\n"
              << "  run <config.json>            使用 JSON 配置文件执行任务\n"
              << "  segment <model> <input> [output_tif] [output_shp]\n"
              << "                               单图分割\n"
              << "  batch <model> <input_dir> <output_dir>\n"
              << "                               批量分割\n"
              << "  generate-config <output>     生成默认配置文件模板\n"
              << "  version                      显示版本信息\n"
              << "  help                         显示帮助信息\n\n"
              << "选项:\n"
              << "  --tile-size <int>            切片大小 (默认: 512)\n"
              << "  --stride <int>               滑窗步长 (默认: 384)\n"
              << "  --blend <none|linear|gaussian> 融合模式 (默认: gaussian)\n"
              << "  --threads <int>              线程数 (默认: 1)\n"
              << "  --no-shp                     不输出矢量文件\n"
              << "  --verbose                    详细日志\n"
              << std::endl;
}

static void PrintVersion() {
    std::cout << "GIS AI 算法库 v0.1.0" << std::endl;
}

static int RunWithConfig(const std::string& config_path) {
    try {
        auto config = gis_ai::TaskConfig::LoadFromFile(config_path);
        gis_ai::Logger::Instance().Initialize(config.log_file,
            config.verbose ? spdlog::level::debug : spdlog::level::info);

        LOG_INFO("加载任务配置: " + config_path);
        auto report = gis_ai::TaskRunner::Execute(config);

        std::cout << report.ToString() << std::endl;
        return report.success ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}

static int RunSegment(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "用法: gis_ai_cli segment <model> <input> [output_tif] [output_shp]" << std::endl;
        return 1;
    }

    gis_ai::TaskConfig config;
    config.task_type = gis_ai::TaskType::SegmentToPolygon;
    config.model_path = argv[2];
    config.input_path = argv[3];

    if (argc >= 5) config.output_tif_path = argv[4];
    if (argc >= 6) config.output_shp_path = argv[5];

    for (int i = 5; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--tile-size" && i + 1 < argc) {
            config.seg_config.tile_size = std::atoi(argv[++i]);
        } else if (arg == "--stride" && i + 1 < argc) {
            config.seg_config.stride = std::atoi(argv[++i]);
        } else if (arg == "--blend" && i + 1 < argc) {
            std::string mode = argv[++i];
            if (mode == "none") config.seg_config.blend_mode = gis_ai::BlendMode::None;
            else if (mode == "linear") config.seg_config.blend_mode = gis_ai::BlendMode::Linear;
            else config.seg_config.blend_mode = gis_ai::BlendMode::Gaussian;
        } else if (arg == "--no-shp") {
            config.task_type = gis_ai::TaskType::Segment;
        } else if (arg == "--verbose") {
            config.verbose = true;
        }
    }

    gis_ai::Logger::Instance().Initialize(config.log_file,
        config.verbose ? spdlog::level::debug : spdlog::level::info);

    auto report = gis_ai::TaskRunner::Execute(config);
    std::cout << report.ToString() << std::endl;
    return report.success ? 0 : 1;
}

static int RunBatch(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "用法: gis_ai_cli batch <model> <input_dir> <output_dir>" << std::endl;
        return 1;
    }

    gis_ai::TaskConfig config;
    config.task_type = gis_ai::TaskType::BatchSegment;
    config.model_path = argv[2];
    config.input_dir = argv[3];
    config.output_dir = argv[4];

    for (int i = 5; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--threads" && i + 1 < argc) {
            config.num_threads = std::atoi(argv[++i]);
        } else if (arg == "--tile-size" && i + 1 < argc) {
            config.seg_config.tile_size = std::atoi(argv[++i]);
        } else if (arg == "--stride" && i + 1 < argc) {
            config.seg_config.stride = std::atoi(argv[++i]);
        } else if (arg == "--verbose") {
            config.verbose = true;
        }
    }

    gis_ai::Logger::Instance().Initialize(config.log_file,
        config.verbose ? spdlog::level::debug : spdlog::level::info);

    auto report = gis_ai::TaskRunner::Execute(config);
    std::cout << report.ToString() << std::endl;
    return report.success ? 0 : 1;
}

static int GenerateConfig(const std::string& output_path) {
    gis_ai::TaskConfig config;
    config.task_type = gis_ai::TaskType::SegmentToPolygon;
    config.model_path = "model.onnx";
    config.input_path = "input.tif";
    config.output_tif_path = "output_seg.tif";
    config.output_shp_path = "output_seg.shp";

    try {
        config.SaveToFile(output_path);
        std::cout << "默认配置文件已生成: " << output_path << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    if (argc < 2) {
        PrintUsage();
        return 0;
    }

    std::string command = argv[1];

    if (command == "run") {
        if (argc < 3) {
            std::cerr << "用法: gis_ai_cli run <config.json>" << std::endl;
            return 1;
        }
        return RunWithConfig(argv[2]);
    } else if (command == "segment") {
        return RunSegment(argc, argv);
    } else if (command == "batch") {
        return RunBatch(argc, argv);
    } else if (command == "generate-config") {
        std::string output = (argc >= 3) ? argv[2] : "task_config.json";
        return GenerateConfig(output);
    } else if (command == "version") {
        PrintVersion();
        return 0;
    } else if (command == "help" || command == "--help" || command == "-h") {
        PrintUsage();
        return 0;
    } else {
        std::cerr << "未知命令: " << command << std::endl;
        PrintUsage();
        return 1;
    }
}
