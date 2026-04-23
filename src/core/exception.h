#ifndef GIS_AI_EXCEPTION_H
#define GIS_AI_EXCEPTION_H

#include <exception>
#include <string>
#include <stdexcept>

namespace gis_ai {

enum class ErrorCode {
    Success = 0,
    IO = 1001,
    ModelLoad = 2001,
    Inference = 2002,
    Algorithm = 3001,
    Config = 4001,
    Memory = 5001,
    InvalidParam = 6001,
    Unknown = 9999
};

class GisAiException : public std::runtime_error {
public:
    GisAiException(ErrorCode code, const std::string& message,
                   const std::string& context = "");

    ErrorCode GetCode() const { return code_; }
    std::string GetContext() const { return context_; }

private:
    ErrorCode code_;
    std::string context_;
};

class GisAiIOException : public GisAiException {
public:
    explicit GisAiIOException(const std::string& msg, const std::string& ctx = "")
        : GisAiException(ErrorCode::IO, msg, ctx) {}
};

class GisAiModelException : public GisAiException {
public:
    explicit GisAiModelException(const std::string& msg, const std::string& ctx = "")
        : GisAiException(ErrorCode::ModelLoad, msg, ctx) {}
};

class GisAiAlgorithmException : public GisAiException {
public:
    explicit GisAiAlgorithmException(const std::string& msg, const std::string& ctx = "")
        : GisAiException(ErrorCode::Algorithm, msg, ctx) {}
};

class GisAiConfigException : public GisAiException {
public:
    explicit GisAiConfigException(const std::string& msg, const std::string& ctx = "")
        : GisAiException(ErrorCode::Config, msg, ctx) {}
};

} // namespace gis_ai

#endif // GIS_AI_EXCEPTION_H
