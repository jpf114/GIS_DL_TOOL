#pragma once

#include <QString>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include "param_card_widget.h"
#include "data_inspector.h"

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

struct ParamText {
    QString displayName;
    QString description;
};

struct FileParamUiConfig {
    bool isOutput = false;
    bool selectDirectory = false;
    bool allowMultiSelect = false;
    QString placeholder;
    QString openFilter;
    QString saveFilter;
    std::string suggestedSuffix;
};

std::vector<ParamSpec> getParamSpecsForPlugin(const std::string& pluginName);

ActionUiConfig getActionUiConfig(const std::string& pluginName, const std::string& actionKey);
bool isKnownAction(const std::string& pluginName, const std::string& actionKey);

std::vector<ParamSpec> buildEffectiveParamSpecs(const std::string& pluginName, const std::string& actionKey);

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

FileParamUiConfig buildFileParamUiConfig(const std::string& pluginName,
                                         const std::string& action,
                                         const std::string& paramKey,
                                         ParamType paramType);

const ParamText* findCommonParamText(const std::string& paramKey);

QString enumDisplayText(const std::string& paramKey, const std::string& rawValue);

QString actionDisplayName(const std::string& pluginName, const std::string& actionKey);

}
