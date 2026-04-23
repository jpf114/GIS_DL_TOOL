#ifndef GIS_AI_LOGGER_H
#define GIS_AI_LOGGER_H

#include <string>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "core/export.h"

namespace gis_ai {

class GIS_AI_API Logger {
public:
    static Logger& Instance();

    void Initialize(const std::string& log_file = "gis_ai.log",
                    spdlog::level::level_enum level = spdlog::level::info);

    std::shared_ptr<spdlog::logger> GetLogger() const { return logger_; }

    void Debug(const std::string& msg);
    void Info(const std::string& msg);
    void Warn(const std::string& msg);
    void Error(const std::string& msg);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::shared_ptr<spdlog::logger> logger_;
};

#define LOG_DEBUG(msg) gis_ai::Logger::Instance().Debug(msg)
#define LOG_INFO(msg) gis_ai::Logger::Instance().Info(msg)
#define LOG_WARN(msg) gis_ai::Logger::Instance().Warn(msg)
#define LOG_ERROR(msg) gis_ai::Logger::Instance().Error(msg)

} // namespace gis_ai

#endif // GIS_AI_LOGGER_H
