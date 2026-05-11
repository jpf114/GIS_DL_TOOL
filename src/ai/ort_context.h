#ifndef GIS_AI_ORT_CONTEXT_H
#define GIS_AI_ORT_CONTEXT_H

#include <memory>
#include <string>
#include <onnxruntime_cxx_api.h>

namespace gis_ai {

class OrtContext {
public:
    static OrtContext& Instance();

    Ort::Env& GetEnv() { return *env_; }
    Ort::SessionOptions& GetSessionOptions() { return *session_options_; }

    OrtLoggingLevel GetLogLevel() const { return log_level_; }
    void SetLogLevel(OrtLoggingLevel level);

private:
    OrtContext();

    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::SessionOptions> session_options_;
    OrtLoggingLevel log_level_ = ORT_LOGGING_LEVEL_WARNING;
};

} // namespace gis_ai

#endif // GIS_AI_ORT_CONTEXT_H
