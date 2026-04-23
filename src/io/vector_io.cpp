#include "io/vector_io.h"
#include "core/logger.h"
#include "core/exception.h"

#include <ogrsf_frmts.h>
#include <gdal_priv.h>

namespace gis_ai {

FeatureType VectorIO::DetectFeatureType(OGRwkbGeometryType geom_type) {
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
			feature.coordinates.push_back(
				{ls->getX(i), ls->getY(i), ls->getZ(i)});
		}
	} else if (flat_type == wkbPolygon) {
		OGRPolygon* poly = geom->toPolygon();
		OGRLinearRing* ring = poly->getExteriorRing();
		if (ring) {
			for (int i = 0; i < ring->getNumPoints(); ++i) {
				feature.coordinates.push_back(
					{ring->getX(i), ring->getY(i), ring->getZ(i)});
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
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".shp") {
		return "ESRI Shapefile";
	}
	return "GeoJSON";
}

std::unique_ptr<VectorData> VectorIO::Load(const std::string& path) {
	GDALDataset* dataset = static_cast<GDALDataset*>(
		OGROpen(path.c_str(), FALSE, nullptr));
	if (!dataset) {
		throw GisAiIOException("Failed to open vector file: " + path, "VectorIO::Load");
	}

	auto data = std::make_unique<VectorData>();

	OGRLayer* layer = dataset->GetLayer(0);
	if (!layer) {
		GDALClose(dataset);
		throw GisAiIOException("No layers found in: " + path, "VectorIO::Load");
	}

	OGRSpatialReference* srs = layer->GetSpatialRef();
	if (srs) {
		char* wkt = nullptr;
		srs->exportToWkt(&wkt);
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

	GDALClose(dataset);
	LOG_INFO("Loaded vector: " + path + " (" + std::to_string(data->features.size()) +
		" features)");
	return data;
}

void VectorIO::Save(const std::string& path, const VectorData& data) {
	std::string driver_name = DetectDriverName(path);
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName(driver_name.c_str());
	if (!driver) {
		throw GisAiIOException(driver_name + " driver not available", "VectorIO::Save");
	}

	OGRSpatialReference* srs = nullptr;
	if (!data.projection.empty()) {
		srs = new OGRSpatialReference(data.projection.c_str());
	}

	OGRwkbGeometryType geom_type = wkbUnknown;
	switch (data.feature_type) {
		case FeatureType::Point:
			geom_type = wkbPoint;
			break;
		case FeatureType::LineString:
			geom_type = wkbLineString;
			break;
		case FeatureType::Polygon:
			geom_type = wkbPolygon;
			break;
	}

	GDALDataset* dataset = driver->Create(path.c_str(), 0, 0, 0, GDT_Unknown, nullptr);
	if (!dataset) {
		if (srs) srs->Release();
		throw GisAiIOException("Failed to create vector file: " + path, "VectorIO::Save");
	}

	OGRLayer* layer = dataset->CreateLayer("features", srs, geom_type, nullptr);
	if (!layer) {
		GDALClose(dataset);
		if (srs) srs->Release();
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
			GDALClose(dataset);
			if (srs) srs->Release();
			throw GisAiIOException("Failed to write feature to: " + path, "VectorIO::Save");
		}
		OGRFeature::DestroyFeature(ogr_feat);
	}

	GDALClose(dataset);
	if (srs) srs->Release();
	LOG_INFO("Saved vector: " + path + " (" + std::to_string(data.features.size()) +
		" features)");
}

} // namespace gis_ai
