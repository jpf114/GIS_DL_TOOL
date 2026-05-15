#pragma once

#include <QString>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include "param_card_widget.h"

namespace gis_ai::gui {

class QtProgressReporter;

enum class DataKind { Raster, Vector, Unknown };

enum class DataOrigin { Input, Output };

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

struct DataAutoFillInfo {
    std::string crs;
    std::string layerName;
    std::array<double, 4> extent = {0, 0, 0, 0};
    bool hasExtent = false;
};

struct DerivedOutputUpdate {
    std::string value;
    std::string autoValue;
    bool shouldApply = false;
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

DataKind detectDataKind(const std::string& path);

bool isSupportedDataPath(const std::string& path);

std::string dataKindDisplayName(DataKind kind);

DataAutoFillInfo inspectDataForAutoFill(const std::string& path);

std::string buildSuggestedOutputPath(const std::string& inputPath,
                                     const std::string& pluginName,
                                     const std::string& action,
                                     const std::string& paramKey);

DerivedOutputUpdate computeDerivedOutputUpdate(const std::string& currentValue,
                                               const std::string& lastAutoValue,
                                               const std::string& primaryPath,
                                               const std::string& pluginName,
                                               const std::string& action,
                                               const std::string& paramKey);

bool shouldAutoFillLayerValue(const std::string& currentValue,
                              const std::string& lastAutoValue,
                              const std::string& suggestedValue);

bool shouldAutoFillExtentValue(const std::optional<std::array<double, 4>>& currentValue,
                               const std::optional<std::array<double, 4>>& lastAutoValue,
                               bool hasSuggestedExtent);

FileParamUiConfig buildFileParamUiConfig(const std::string& pluginName,
                                         const std::string& action,
                                         const std::string& paramKey,
                                         ParamType paramType);

const ParamText* findCommonParamText(const std::string& paramKey);

QString enumDisplayText(const std::string& paramKey, const std::string& rawValue);

QString actionDisplayName(const std::string& pluginName, const std::string& actionKey);

}
