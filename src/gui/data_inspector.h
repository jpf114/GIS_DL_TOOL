#pragma once

#include <QString>
#include <array>
#include <optional>
#include <string>

namespace gis_ai::gui {

enum class DataKind { Raster, Vector, Unknown };

enum class DataOrigin { Input, Output };

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

std::string defaultSuffixForOutput(const std::string& pluginName,
                                   const std::string& action,
                                   const std::string& paramKey,
                                   const std::string& inputExt);

}
