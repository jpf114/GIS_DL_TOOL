#pragma once

#include <map>
#include <string>

#include "param_card_widget.h"

namespace gis_ai::gui {

class QtProgressReporter;

bool executeAction(const std::string& pluginName, const std::string& actionKey,
                   const std::map<std::string, ParamValue>& params,
                   QtProgressReporter& reporter);

std::string localizeResultMessage(const std::string& message);

}
