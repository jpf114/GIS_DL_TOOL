#include "ai/ort_context.h"
#include "core/logger.h"

namespace gis_ai {

OrtContext& OrtContext::Instance() {
    static OrtContext instance;
    return instance;
}

OrtContext::OrtContext() {
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "gis_ai");
    session_options_ = std::make_unique<Ort::SessionOptions>();
    session_options_->SetIntraOpNumThreads(0);
    session_options_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_DISABLE_ALL);

    LOG_INFO("ONNX Runtime context initialized");
}

void OrtContext::SetLogLevel(OrtLoggingLevel level) {
    env_ = std::make_unique<Ort::Env>(level, "gis_ai");
    LOG_INFO("ONNX Runtime log level updated");
}

} // namespace gis_ai
