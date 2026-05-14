#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <cpl_conv.h>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

namespace {

class TestDataGenerationLock {
public:
    explicit TestDataGenerationLock(std::filesystem::path lock_path)
        : lock_path_(std::move(lock_path)) {}

    bool acquire(std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            std::error_code ec;
            if (std::filesystem::create_directory(lock_path_, ec)) {
                owns_lock_ = true;
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return false;
    }

    ~TestDataGenerationLock() {
        if (!owns_lock_) {
            return;
        }
        std::error_code ec;
        std::filesystem::remove(lock_path_, ec);
    }

private:
    std::filesystem::path lock_path_;
    bool owns_lock_ = false;
};

void remove_if_exists(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return;
    }
    std::filesystem::permissions(
        path,
        std::filesystem::perms::owner_all,
        std::filesystem::perm_options::add,
        ec);
    ec.clear();
    std::filesystem::remove(path, ec);
}

void remove_shapefile_set(const std::filesystem::path& shp_path) {
    static const char* kExtensions[] = {
        ".shp", ".shx", ".dbf", ".prj", ".cpg", ".qix"
    };
    const auto stem = shp_path.parent_path() / shp_path.stem();
    for (const char* ext : kExtensions) {
        remove_if_exists(stem.string() + ext);
    }
}

}  // namespace

int main() {
    std::filesystem::create_directories("test_data/raster");
    std::filesystem::create_directories("test_data/vector");

    TestDataGenerationLock lock("test_data/.generate_test_data.lock");
    if (!lock.acquire(std::chrono::seconds(30))) {
        std::cerr << "Timed out waiting for test data generation lock\n";
        return 1;
    }

    GDALAllRegister();

    auto* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (!driver) { std::cerr << "No GTiff driver\n"; return 1; }

    remove_if_exists("test_data/raster/test_100x100.tif");
    char** create_opts = nullptr;
    create_opts = CSLSetNameValue(create_opts, "INTERLEAVE", "PIXEL");
    create_opts = CSLSetNameValue(create_opts, "PHOTOMETRIC", "RGB");
    auto* ds = driver->Create("test_data/raster/test_100x100.tif", 100, 100, 3, GDT_Byte, create_opts);
    CSLDestroy(create_opts);
    if (!ds) { std::cerr << "Failed to create tif\n"; return 1; }

    double gt[6] = {116.0, 0.001, 0.0, 40.0, 0.0, -0.001};
    ds->SetGeoTransform(gt);
    OGRSpatialReference srs;
    srs.importFromEPSG(4326);
    char* wkt = nullptr;
    srs.exportToWkt(&wkt);
    ds->SetProjection(wkt);
    CPLFree(wkt);

    for (int b = 1; b <= 3; ++b) {
        auto* band = ds->GetRasterBand(b);
        band->SetColorInterpretation(
            b == 1 ? GCI_RedBand : (b == 2 ? GCI_GreenBand : GCI_BlueBand));
        std::vector<unsigned char> data(100 * 100);
        for (int i = 0; i < 100 * 100; ++i) {
            data[i] = static_cast<unsigned char>((i + (b - 1) * 37) % 256);
        }
        band->RasterIO(GF_Write, 0, 0, 100, 100, data.data(), 100, 100, GDT_Byte, 0, 0);
    }
    GDALClose(ds);
    std::cout << "Created test GeoTIFF: test_data/raster/test_100x100.tif" << std::endl;

    auto* shp_driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    if (!shp_driver) { std::cerr << "No Shapefile driver\n"; return 1; }

    remove_shapefile_set("test_data/vector/test_polygons.shp");
    auto* shp_ds = shp_driver->Create("test_data/vector/test_polygons.shp", 0, 0, 0, GDT_Unknown, nullptr);
    if (!shp_ds) { std::cerr << "Failed to create shp\n"; return 1; }

    OGRLayer* layer = shp_ds->CreateLayer("polygons", &srs, wkbPolygon, nullptr);
    OGRFieldDefn name_field("name", OFTString);
    layer->CreateField(&name_field);

    double centers[][2] = {{116.05, 39.95}, {116.07, 39.97}};
    const char* names[] = {"poly_0", "poly_1"};

    for (int idx = 0; idx < 2; ++idx) {
        OGRFeature feat(layer->GetLayerDefn());
        feat.SetField("name", names[idx]);
        double cx = centers[idx][0], cy = centers[idx][1];
        double half = 0.02;
        OGRLinearRing ring;
        ring.addPoint(cx - half, cy - half);
        ring.addPoint(cx + half, cy - half);
        ring.addPoint(cx + half, cy + half);
        ring.addPoint(cx - half, cy + half);
        ring.addPoint(cx - half, cy - half);
        OGRPolygon poly;
        poly.addRing(&ring);
        feat.SetGeometry(&poly);
        layer->CreateFeature(&feat);
    }
    GDALClose(shp_ds);
    std::cout << "Created test Shapefile: test_data/vector/test_polygons.shp" << std::endl;

    auto* json_driver = GetGDALDriverManager()->GetDriverByName("GeoJSON");
    if (!json_driver) { std::cerr << "No GeoJSON driver\n"; return 1; }

    remove_if_exists("test_data/vector/test_points.geojson");
    auto* json_ds = json_driver->Create("test_data/vector/test_points.geojson", 0, 0, 0, GDT_Unknown, nullptr);
    if (!json_ds) { std::cerr << "Failed to create geojson\n"; return 1; }

    OGRLayer* pt_layer = json_ds->CreateLayer("points", &srs, wkbPoint, nullptr);
    OGRFieldDefn val_field("value", OFTReal);
    pt_layer->CreateField(&val_field);

    for (int i = 0; i < 10; ++i) {
        OGRFeature feat(pt_layer->GetLayerDefn());
        feat.SetField("value", i * 10.0);
        OGRPoint pt(116.0 + i * 0.01, 39.9 + i * 0.005);
        feat.SetGeometry(&pt);
        pt_layer->CreateFeature(&feat);
    }
    GDALClose(json_ds);
    std::cout << "Created test GeoJSON: test_data/vector/test_points.geojson" << std::endl;

    std::cout << "\nAll test data created successfully!" << std::endl;
    return 0;
}
