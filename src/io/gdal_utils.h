#ifndef GIS_AI_GDAL_UTILS_H
#define GIS_AI_GDAL_UTILS_H

#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <string>
#include <memory>
#include <cctype>

namespace gis_ai {

struct GdalDatasetCloser {
    void operator()(GDALDataset* ds) const {
        if (ds) GDALClose(ds);
    }
};

using GdalDatasetPtr = std::unique_ptr<GDALDataset, GdalDatasetCloser>;

struct OgrSpatialRefReleaser {
    void operator()(OGRSpatialReference* srs) const {
        if (srs) srs->Release();
    }
};

using OgrSrsPtr = std::unique_ptr<OGRSpatialReference, OgrSpatialRefReleaser>;

inline GdalDatasetPtr MakeGdalDataset(GDALDataset* ds) {
    return GdalDatasetPtr(ds);
}

inline OgrSrsPtr MakeOgrSrs(OGRSpatialReference* srs) {
    return OgrSrsPtr(srs);
}

inline void EnsureGDALInitialized() {
    static bool initialized = false;
    if (!initialized) {
        GDALAllRegister();
        initialized = true;
    }
}

inline std::string GetExtensionLower(const std::string& path) {
    std::string ext;
    for (auto it = path.rbegin(); it != path.rend() && *it != '.' && *it != '/' && *it != '\\'; ++it) {
        ext.insert(ext.begin(), static_cast<char>(std::tolower(static_cast<unsigned char>(*it))));
    }
    if (!ext.empty()) ext = "." + ext;
    return ext;
}

} // namespace gis_ai

#endif // GIS_AI_GDAL_UTILS_H
