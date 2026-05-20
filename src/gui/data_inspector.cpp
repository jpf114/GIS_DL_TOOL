#include "data_inspector.h"

#include <QCoreApplication>

#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <unordered_set>

namespace gis_ai::gui {

namespace {

std::string lowerExtension(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return ext;
}

std::string sanitizeSuffixPart(const std::string& value) {
    std::string sanitized;
    sanitized.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch)) {
            sanitized.push_back(static_cast<char>(std::tolower(ch)));
        } else if (ch == '-' || ch == '_') {
            sanitized.push_back('_');
        }
    }
    return sanitized;
}

std::string firstInputPath(const std::string& rawPath) {
    const auto pos = rawPath.find(',');
    if (pos == std::string::npos) {
        auto trimmed = rawPath;
        while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
            trimmed.erase(trimmed.begin());
        while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t'))
            trimmed.pop_back();
        return trimmed;
    }
    auto trimmed = rawPath.substr(0, pos);
    while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
        trimmed.erase(trimmed.begin());
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t'))
        trimmed.pop_back();
    return trimmed;
}

std::string spatialReferenceText(const OGRSpatialReference* srs) {
    if (!srs) return {};
    const char* authName = srs->GetAuthorityName(nullptr);
    const char* authCode = srs->GetAuthorityCode(nullptr);
    if (authName && authCode) {
        return std::string(authName) + ":" + authCode;
    }
    char* wkt = nullptr;
    OGRSpatialReference cloned(*srs);
    if (cloned.exportToWkt(&wkt) != OGRERR_NONE || !wkt) return {};
    std::string text = wkt;
    CPLFree(wkt);
    return text;
}

struct DatasetCloser {
    void operator()(GDALDataset* ds) const {
        if (ds) GDALClose(ds);
    }
};

}

DataKind detectDataKind(const std::string& path) {
    static const std::unordered_set<std::string> rasterExts = {
        ".tif", ".tiff", ".img", ".vrt", ".png", ".jpg", ".jpeg", ".bmp"
    };
    static const std::unordered_set<std::string> vectorExts = {
        ".shp", ".geojson", ".json", ".gpkg", ".kml", ".csv"
    };

    const std::string ext = lowerExtension(path);
    if (rasterExts.count(ext) > 0) return DataKind::Raster;
    if (vectorExts.count(ext) > 0) return DataKind::Vector;
    return DataKind::Unknown;
}

bool isSupportedDataPath(const std::string& path) {
    return detectDataKind(path) != DataKind::Unknown;
}

QString dataKindDisplayName(DataKind kind) {
    switch (kind) {
        case DataKind::Raster: return QCoreApplication::translate("DataInspector", "栅格");
        case DataKind::Vector: return QCoreApplication::translate("DataInspector", "矢量");
        default: return QCoreApplication::translate("DataInspector", "未知");
    }
}

DataAutoFillInfo inspectDataForAutoFill(const std::string& path) {
    DataAutoFillInfo info;
    const std::string normalizedPath = firstInputPath(path);
    const DataKind kind = detectDataKind(normalizedPath);

    if (kind == DataKind::Raster) {
        std::unique_ptr<GDALDataset, DatasetCloser> ds(
            static_cast<GDALDataset*>(GDALOpen(normalizedPath.c_str(), GA_ReadOnly)));
        if (!ds) return info;

        info.crs = spatialReferenceText(ds->GetSpatialRef());

        double gt[6] = {};
        if (ds->GetGeoTransform(gt) == CE_None) {
            const double minX = gt[0];
            const double maxY = gt[3];
            const double maxX = gt[0] + gt[1] * ds->GetRasterXSize() + gt[2] * ds->GetRasterYSize();
            const double minY = gt[3] + gt[4] * ds->GetRasterXSize() + gt[5] * ds->GetRasterYSize();
            info.extent = {
                std::min(minX, maxX),
                std::min(minY, maxY),
                std::max(minX, maxX),
                std::max(minY, maxY)
            };
            info.hasExtent = true;
        }
        return info;
    }

    if (kind == DataKind::Vector) {
        std::unique_ptr<GDALDataset, DatasetCloser> ds(
            static_cast<GDALDataset*>(GDALOpenEx(normalizedPath.c_str(),
                GDAL_OF_VECTOR | GDAL_OF_READONLY, nullptr, nullptr, nullptr)));
        if (!ds) return info;

        auto* layer = ds->GetLayer(0);
        if (!layer) return info;

        if (lowerExtension(normalizedPath) != ".shp") {
            info.layerName = layer->GetName();
        }
        info.crs = spatialReferenceText(layer->GetSpatialRef());

        OGREnvelope envelope{};
        if (layer->GetExtent(&envelope, TRUE) == OGRERR_NONE) {
            info.extent = {envelope.MinX, envelope.MinY, envelope.MaxX, envelope.MaxY};
            info.hasExtent = true;
        }
    }

    return info;
}

std::string defaultSuffixForOutput(const std::string& pluginName,
                                   const std::string& action,
                                   const std::string& paramKey,
                                   const std::string& inputExt) {
    if (pluginName == "segment") {
        if (paramKey == "output_tif") return ".tif";
        if (paramKey == "output_shp") return ".shp";
        return inputExt;
    }
    if (pluginName == "inference") return ".tif";
    if (pluginName == "preprocess") return ".tif";
    if (pluginName == "vector") {
        if (action == "vector_simplify" || action == "vector_buffer" || action == "vector_clip")
            return ".gpkg";
        return inputExt;
    }
    if (pluginName == "raster") return ".tif";
    if (pluginName == "batch") return {};
    return inputExt;
}

std::string buildSuggestedOutputPath(const std::string& inputPath,
                                     const std::string& pluginName,
                                     const std::string& action,
                                     const std::string& paramKey) {
    namespace fs = std::filesystem;

    fs::path input = fs::path(firstInputPath(inputPath));
    if (input.empty()) return {};

    const std::string pluginSuffix = sanitizeSuffixPart(pluginName);
    const std::string actionSuffix = sanitizeSuffixPart(action);

    std::string suffix = "result";
    if (!pluginSuffix.empty()) {
        suffix = pluginSuffix;
        if (!actionSuffix.empty()) suffix += "_" + actionSuffix;
    } else if (!actionSuffix.empty()) {
        suffix = actionSuffix;
    }

    std::string inputExtLower = lowerExtension(input.extension().string());
    const std::string resolvedSuffix = defaultSuffixForOutput(
        pluginName, action, paramKey, inputExtLower);

    if (resolvedSuffix.empty()) {
        return (input.parent_path() / fs::path(input.stem().string() + "_" + suffix)).generic_string();
    }

    const fs::path suggested = input.parent_path() /
        fs::path(input.stem().string() + "_" + suffix + input.extension().string());

    fs::path rewritten = suggested;
    rewritten.replace_extension(resolvedSuffix);
    return rewritten.generic_string();
}

DerivedOutputUpdate computeDerivedOutputUpdate(const std::string& currentValue,
                                               const std::string& lastAutoValue,
                                               const std::string& primaryPath,
                                               const std::string& pluginName,
                                               const std::string& action,
                                               const std::string& paramKey) {
    DerivedOutputUpdate update;
    if (primaryPath.empty()) return update;

    std::string suggestedValue = buildSuggestedOutputPath(primaryPath, pluginName, action, paramKey);

    const bool valueWasAuto = !lastAutoValue.empty() && currentValue == lastAutoValue;
    update.value = suggestedValue;
    update.autoValue = suggestedValue;
    update.shouldApply = (currentValue.empty() || valueWasAuto) && currentValue != suggestedValue;
    return update;
}

bool shouldAutoFillLayerValue(const std::string& currentValue,
                              const std::string& lastAutoValue,
                              const std::string& suggestedValue) {
    if (suggestedValue.empty()) return false;
    const bool valueWasAuto = !lastAutoValue.empty() && currentValue == lastAutoValue;
    return (currentValue.empty() || valueWasAuto) && currentValue != suggestedValue;
}

bool shouldAutoFillExtentValue(const std::optional<std::array<double, 4>>& currentValue,
                               const std::optional<std::array<double, 4>>& lastAutoValue,
                               bool hasSuggestedExtent) {
    if (!hasSuggestedExtent) return false;
    auto isZero = [](const std::array<double, 4>& e) -> bool {
        return e[0] == 0.0 && e[1] == 0.0 && e[2] == 0.0 && e[3] == 0.0;
    };
    const bool extentWasAuto = currentValue.has_value() && lastAutoValue.has_value()
        && *currentValue == *lastAutoValue;
    bool currentIsZeroOrEmpty = !currentValue.has_value() ||
        (currentValue.has_value() && isZero(*currentValue));
    return currentIsZeroOrEmpty || extentWasAuto;
}

}
