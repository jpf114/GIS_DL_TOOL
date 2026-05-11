#include "ai/ort_context.h"
#include "core/logger.h"

namespace gis_ai {

OrtContext& OrtContext::Instance() {
    static OrtContext instance;
    return instance;
}

OrtContext::OrtContext()
    : log_level_(ORT_LOGGING_LEVEL_WARNING) {
    env_ = std::make_unique<Ort::Env>(log_level_, "gis_ai");
    session_options_ = std::make_unique<Ort::SessionOptions>();
    session_options_->SetIntraOpNumThreads(0);
    session_options_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    LOG_INFO("ONNX Runtime context initialized (graph optimization: EXTENDED)");
}

void OrtContext::SetLogLevel(OrtLoggingLevel level) {
    log_level_ = level;
    LOG_WARN("ONNX Runtime log level set to " + std::to_string(static_cast<int>(level)) +
             ". Note: existing sessions retain the previous Env. Rebuild sessions to apply fully.");
}

} // namespace gis_ai
