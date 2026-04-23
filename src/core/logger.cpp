#include "core/logger.h"

namespace gis_ai {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::Initialize(const std::string& log_file,
                        spdlog::level::level_enum level) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(level);

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
    file_sink->set_level(level);

    logger_ = std::make_shared<spdlog::logger>(
        "gis_ai", spdlog::sinks_init_list{console_sink, file_sink});
    logger_->set_level(level);
    logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    spdlog::set_default_logger(logger_);
}

void Logger::Debug(const std::string& msg) {
    if (logger_) logger_->debug(msg);
}

void Logger::Info(const std::string& msg) {
    if (logger_) logger_->info(msg);
}

void Logger::Warn(const std::string& msg) {
    if (logger_) logger_->warn(msg);
}

void Logger::Error(const std::string& msg) {
    if (logger_) logger_->error(msg);
}

} // namespace gis_ai
