#include "core/exception.h"

namespace gis_ai {

GisAiException::GisAiException(ErrorCode code, const std::string& message,
                               const std::string& context)
    : std::runtime_error(
          context.empty()
              ? message
              : message + " [context: " + context + "]"),
      code_(code),
      context_(context) {}

} // namespace gis_ai
