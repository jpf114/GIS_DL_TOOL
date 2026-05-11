#include "io/vector_io.h"
#include "io/gdal_utils.h"
#include "core/logger.h"
#include "core/exception.h"

#include <ogrsf_frmts.h>
#include <gdal_priv.h>

namespace gis_ai {

static FeatureType DetectFeatureType(OGRwkbGeometryType geom_type) {
    switch (wkbFlatten(geom_type)) {
        case wkbPoint:
        case wkbMultiPoint:
            return FeatureType::Point;
        case wkbLineString:
        case wkbMultiLineString:
            return FeatureType::LineString;
        case wkbPolygon:
        case wkbMultiPolygon:
            return FeatureType::Polygon;
        default:
            return FeatureType::Point;
    }
}

static void ExtractCoordinates(OGRGeometry* geom, Feature& feature) {
    OGRwkbGeometryType flat_type = wkbFlatten(geom->getGeometryType());

    if (flat_type == wkbPoint) {
        OGRPoint* pt = geom->toPoint();
        feature.coordinates.push_back({pt->getX(), pt->getY(), pt->getZ()});
    } else if (flat_type == wkbLineString) {
        OGRLineString* ls = geom->toLineString();
        for (int i = 0; i < ls->getNumPoints(); ++i) {
            feature.coordinates.push_back({ls->getX(i), ls->getY(i), ls->getZ(i)});
        }
    } else if (flat_type == wkbPolygon) {
        OGRPolygon* poly = geom->toPolygon();
        OGRLinearRing* ring = poly->getExteriorRing();
        if (ring) {
            for (int i = 0; i < ring->getNumPoints(); ++i) {
                feature.coordinates.push_back({ring->getX(i), ring->getY(i), ring->getZ(i)});
            }
        }
    } else if (flat_type == wkbMultiPoint || flat_type == wkbMultiLineString ||
               flat_type == wkbMultiPolygon) {
        OGRGeometryCollection* gc = geom->toGeometryCollection();
        for (int i = 0; i < gc->getNumGeometries(); ++i) {
            ExtractCoordinates(gc->getGeometryRef(i), feature);
        }
    }
}

static std::string DetectDriverName(const std::string& path) {
    std::string ext = GetExtensionLower(path);
    if (ext == ".shp") return "ESRI Shapefile";
    if (ext == ".gpkg") return "GPKG";
    return "GeoJSON";
}

static std::unique_ptr<VectorData> LoadLayerFromDataset(GDALDataset* dataset, OGRLayer* layer) {
    auto data = std::make_unique<VectorData>();

    const OGRSpatialReference* srs = layer->GetSpatialRef();
    if (srs) {
        char* wkt = nullptr;
        const_cast<OGRSpatialReference*>(srs)->exportToWkt(&wkt);
        if (wkt) {
            data->projection = wkt;
            CPLFree(wkt);
        }
    }

    OGRFeatureDefn* feat_def = layer->GetLayerDefn();
    data->feature_type = DetectFeatureType(feat_def->GetGeomType());

    layer->ResetReading();
    while (OGRFeature* ogr_feat = layer->GetNextFeature()) {
        Feature feature;
        feature.type = DetectFeatureType(feat_def->GetGeomType());

        OGRGeometry* geom = ogr_feat->GetGeometryRef();
        if (geom) {
            ExtractCoordinates(geom, feature);
        }

        for (int i = 0; i < feat_def->GetFieldCount(); ++i) {
            OGRFieldDefn* field_def = feat_def->GetFieldDefn(i);
            std::string name = field_def->GetNameRef();
            if (!ogr_feat->IsFieldSet(i)) continue;

            switch (field_def->GetType()) {
                case OFTInteger:
                    feature.attributes[name] = ogr_feat->GetFieldAsInteger(i);
                    break;
                case OFTReal:
                    feature.attributes[name] = ogr_feat->GetFieldAsDouble(i);
                    break;
                case OFTString:
                    feature.attributes[name] = std::string(ogr_feat->GetFieldAsString(i));
                    break;
                default:
                    feature.attributes[name] = std::string(ogr_feat->GetFieldAsString(i));
                    break;
            }
        }

        data->features.push_back(std::move(feature));
        OGRFeature::DestroyFeature(ogr_feat);
    }

    return data;
}

std::vector<LayerInfo> VectorIO::ListLayers(const std::string& path) {
    EnsureGDALInitialized();
    auto dataset = MakeGdalDataset(
        reinterpret_cast<GDALDataset*>(OGROpen(path.c_str(), FALSE, nullptr)));
    if (!dataset) {
        throw GisAiIOException("Failed to open vector file: " + path, "VectorIO::ListLayers");
    }

    std::vector<LayerInfo> layers;
    int layer_count = dataset->GetLayerCount();
    for (int i = 0; i < layer_count; ++i) {
        OGRLayer* layer = dataset->GetLayer(i);
        if (!layer) continue;

        LayerInfo info;
        info.name = layer->GetName();
        info.feature_count = static_cast<int>(layer->GetFeatureCount(FALSE));
        info.feature_type = DetectFeatureType(layer->GetLayerDefn()->GetGeomType());
        layers.push_back(info);
    }

    return layers;
}

std::unique_ptr<VectorData> VectorIO::Load(const std::string& path, const std::string& layer_name) {
    EnsureGDALInitialized();
    auto dataset = MakeGdalDataset(
        reinterpret_cast<GDALDataset*>(OGROpen(path.c_str(), FALSE, nullptr)));
    if (!dataset) {
        throw GisAiIOException("Failed to open vector file: " + path, "VectorIO::Load");
    }

    OGRLayer* layer = nullptr;
    if (!layer_name.empty()) {
        layer = dataset->GetLayerByName(layer_name.c_str());
        if (!layer) {
            throw GisAiIOException("Layer '" + layer_name + "' not found in: " + path, "VectorIO::Load");
        }
    } else {
        layer = dataset->GetLayer(0);
    }

    if (!layer) {
        throw GisAiIOException("No layers found in: " + path, "VectorIO::Load");
    }

    auto data = LoadLayerFromDataset(dataset.get(), layer);
    LOG_INFO("Loaded vector: " + path + " (" + std::to_string(data->features.size()) + " features)");
    return data;
}

std::unique_ptr<VectorData> VectorIO::LoadLayer(const std::string& path, int layer_index) {
    EnsureGDALInitialized();
    auto dataset = MakeGdalDataset(
        reinterpret_cast<GDALDataset*>(OGROpen(path.c_str(), FALSE, nullptr)));
    if (!dataset) {
        throw GisAiIOException("Failed to open vector file: " + path, "VectorIO::LoadLayer");
    }

    OGRLayer* layer = dataset->GetLayer(layer_index);
    if (!layer) {
        throw GisAiIOException("Layer index " + std::to_string(layer_index) + " not found in: " + path,
            "VectorIO::LoadLayer");
    }

    auto data = LoadLayerFromDataset(dataset.get(), layer);
    LOG_INFO("Loaded vector layer " + std::to_string(layer_index) + " from: " + path +
        " (" + std::to_string(data->features.size()) + " features)");
    return data;
}

void VectorIO::Save(const VectorData& data, const std::string& path) {
    EnsureGDALInitialized();
    std::string driver_name = DetectDriverName(path);
    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName(driver_name.c_str());
    if (!driver) {
        throw GisAiIOException(driver_name + " driver not available", "VectorIO::Save");
    }

    OgrSrsPtr srs;
    if (!data.projection.empty()) {
        srs = MakeOgrSrs(new OGRSpatialReference(data.projection.c_str()));
    }

    OGRwkbGeometryType geom_type = wkbUnknown;
    switch (data.feature_type) {
        case FeatureType::Point:     geom_type = wkbPoint25D; break;
        case FeatureType::LineString: geom_type = wkbLineString25D; break;
        case FeatureType::Polygon:   geom_type = wkbPolygon25D; break;
    }

    char** options = nullptr;

    auto dataset = MakeGdalDataset(
        driver->Create(path.c_str(), 0, 0, 0, GDT_Unknown, options));
    CSLDestroy(options);

    if (!dataset) {
        throw GisAiIOException("Failed to create vector file: " + path, "VectorIO::Save");
    }

    std::string layer_name = (driver_name == "GPKG") ? "features" : "features";
    OGRLayer* layer = dataset->CreateLayer(layer_name.c_str(), srs.get(), geom_type, nullptr);
    if (!layer) {
        throw GisAiIOException("Failed to create layer in: " + path, "VectorIO::Save");
    }

    if (!data.features.empty()) {
        for (const auto& [key, val] : data.features[0].attributes) {
            OGRFieldType ft = std::holds_alternative<int>(val) ? OFTInteger :
                std::holds_alternative<double>(val) ? OFTReal : OFTString;
            OGRFieldDefn field(key.c_str(), ft);
            if (layer->CreateField(&field) != OGRERR_NONE) {
                LOG_WARN("Failed to create field: " + key);
            }
        }
    }

    for (const auto& feature : data.features) {
        OGRFeature* ogr_feat = OGRFeature::CreateFeature(layer->GetLayerDefn());

        OGRGeometry* geom = nullptr;
        switch (feature.type) {
            case FeatureType::Point: {
                if (!feature.coordinates.empty()) {
                    const auto& c = feature.coordinates[0];
                    geom = new OGRPoint(c.x, c.y, c.z);
                }
                break;
            }
            case FeatureType::LineString: {
                auto* ls = new OGRLineString();
                for (const auto& c : feature.coordinates) {
                    ls->addPoint(c.x, c.y, c.z);
                }
                geom = ls;
                break;
            }
            case FeatureType::Polygon: {
                auto* ring = new OGRLinearRing();
                for (const auto& c : feature.coordinates) {
                    ring->addPoint(c.x, c.y, c.z);
                }
                if (!feature.coordinates.empty()) {
                    const auto& first = feature.coordinates[0];
                    ring->addPoint(first.x, first.y, first.z);
                }
                auto* poly = new OGRPolygon();
                poly->addRing(ring);
                delete ring;
                geom = poly;
                break;
            }
        }

        if (geom) {
            ogr_feat->SetGeometry(geom);
            delete geom;
        }

        for (const auto& [key, val] : feature.attributes) {
            int idx = ogr_feat->GetFieldIndex(key.c_str());
            if (idx < 0) continue;
            if (std::holds_alternative<int>(val)) {
                ogr_feat->SetField(idx, std::get<int>(val));
            } else if (std::holds_alternative<double>(val)) {
                ogr_feat->SetField(idx, std::get<double>(val));
            } else {
                ogr_feat->SetField(idx, std::get<std::string>(val).c_str());
            }
        }

        if (layer->CreateFeature(ogr_feat) != OGRERR_NONE) {
            OGRFeature::DestroyFeature(ogr_feat);
            throw GisAiIOException("Failed to write feature to: " + path, "VectorIO::Save");
        }
        OGRFeature::DestroyFeature(ogr_feat);
    }

    LOG_INFO("Saved vector: " + path + " (" + std::to_string(data.features.size()) + " features)");
}

} // namespace gis_ai
