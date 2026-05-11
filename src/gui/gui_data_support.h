#pragma once

#include <QString>
#include <set>
#include <string>
#include <vector>
#include "param_card_widget.h"

namespace gis_ai::gui {

class QtProgressReporter;

struct ActionUiConfig {
    QString displayName;
    QString description;
    std::set<std::string> visibleKeys;
    std::set<std::string> requiredKeys;
};

std::vector<ParamSpec> getParamSpecsForPlugin(const std::string& pluginName);

ActionUiConfig getActionUiConfig(const std::string& pluginName, const std::string& actionKey);

std::vector<ParamSpec> buildEffectiveParamSpecs(const std::string& pluginName, const std::string& actionKey);

bool executeAction(const std::string& pluginName, const std::string& actionKey,
                   const std::map<std::string, ParamValue>& params,
                   QtProgressReporter& reporter);

}
