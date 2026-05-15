#ifndef GIS_AI_EXCEPTION_H
#define GIS_AI_EXCEPTION_H

#include <exception>
#include <string>
#include <stdexcept>
#include "core/export.h"

namespace gis_ai {

enum class GIS_AI_API ErrorCode {
    Success = 0,
    IO = 1001,
    FileNotFound = 1002,
    FileReadError = 1003,
    FileWriteError = 1004,
    ModelLoad = 2001,
    Inference = 2002,
    Algorithm = 3001,
    Config = 4001,
    ConfigParseError = 4002,
    ConfigValidationError = 4003,
    Memory = 5001,
    InvalidParam = 6001,
    MissingRequiredParam = 6002,
    TaskExecution = 7001,
    TaskCancelled = 7002,
    Unknown = 9999
};

GIS_AI_API std::string ErrorCodeToString(ErrorCode code);
GIS_AI_API int ErrorCodeToInt(ErrorCode code);

class GIS_AI_API GisAiException : public std::runtime_error {
public:
    GisAiException(ErrorCode code, const std::string& message,
                   const std::string& context = "");

    ErrorCode GetCode() const { return code_; }
    std::string GetContext() const { return context_; }

private:
    ErrorCode code_;
    std::string context_;
};

class GIS_AI_API GisAiIOException : public GisAiException {
public:
    explicit GisAiIOException(const std::string& msg, const std::string& ctx = "")
        : GisAiException(ErrorCode::IO, msg, ctx) {}
};

class GIS_AI_API GisAiModelException : public GisAiException {
public:
    explicit GisAiModelException(const std::string& msg, const std::string& ctx = "")
        : GisAiException(ErrorCode::ModelLoad, msg, ctx) {}
};

class GIS_AI_API GisAiAlgorithmException : public GisAiException {
public:
    explicit GisAiAlgorithmException(const std::string& msg, const std::string& ctx = "")
        : GisAiException(ErrorCode::Algorithm, msg, ctx) {}
};

class GIS_AI_API GisAiConfigException : public GisAiException {
public:
    explicit GisAiConfigException(const std::string& msg, const std::string& ctx = "")
        : GisAiException(ErrorCode::Config, msg, ctx) {}
};

} // namespace gis_ai

#endif // GIS_AI_EXCEPTION_H
