#pragma once

#include <QString>
#include <optional>
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

struct ActionValidationIssue {
    std::string key;
    QString message;
};

struct ExecuteButtonState {
    bool enabled;
    QString tooltip;
    QString badgeText;
    QString badgeStyle;
};

std::vector<ParamSpec> getParamSpecsForPlugin(const std::string& pluginName);

ActionUiConfig getActionUiConfig(const std::string& pluginName, const std::string& actionKey);

std::vector<ParamSpec> buildEffectiveParamSpecs(const std::string& pluginName, const std::string& actionKey);

bool executeAction(const std::string& pluginName, const std::string& actionKey,
                   const std::map<std::string, ParamValue>& params,
                   QtProgressReporter& reporter);

std::string localizeResultMessage(const std::string& message);

std::optional<ActionValidationIssue> validateActionSpecificParams(
    const std::string& pluginName,
    const std::string& actionKey,
    const std::map<std::string, ParamValue>& params);

ExecuteButtonState buildExecuteButtonState(bool hasSelection,
                                           const QString& validationMessage);

std::string resolveHighlightedParamKey(
    bool hasSelection,
    const std::vector<ParamSpec>& specs,
    const std::map<std::string, ParamValue>& params,
    const std::optional<ActionValidationIssue>& actionIssue);

}
