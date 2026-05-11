#include "gdal_config.h"

#include <QApplication>
#include <QDir>
#include <gdal_priv.h>
#include <cpl_conv.h>

namespace gis_ai::gui {

namespace {

QString findDataDir(const QString& relativePath) {
    QDir appDir(QApplication::applicationDirPath());

    QDir dir = appDir;
    for (int i = 0; i < 6; ++i) {
        QString candidate = dir.absoluteFilePath(relativePath);
        if (QDir(candidate).exists()) {
            return candidate;
        }
        if (!dir.cdUp()) break;
    }

    QDir buildDir = appDir;
    if (buildDir.cdUp()) {
        QString candidate = buildDir.absoluteFilePath(relativePath);
        if (QDir(candidate).exists()) {
            return candidate;
        }
    }

    return {};
}

}

void configureGdalRuntime() {
    GDALAllRegister();

    QString gdalDataDir;
    if (qEnvironmentVariableIsSet("GDAL_DATA")) {
        gdalDataDir = QString::fromUtf8(qgetenv("GDAL_DATA"));
    }
    if (gdalDataDir.isEmpty()) {
        gdalDataDir = findDataDir(QStringLiteral("share/gdal"));
    }
    if (!gdalDataDir.isEmpty() && QDir(gdalDataDir).exists()) {
        CPLSetConfigOption("GDAL_DATA", gdalDataDir.toUtf8().constData());
    }

    QString projDataDir;
    if (qEnvironmentVariableIsSet("PROJ_DATA")) {
        projDataDir = QString::fromUtf8(qgetenv("PROJ_DATA"));
    }
    if (projDataDir.isEmpty() && qEnvironmentVariableIsSet("PROJ_LIB")) {
        projDataDir = QString::fromUtf8(qgetenv("PROJ_LIB"));
    }
    if (projDataDir.isEmpty()) {
        projDataDir = findDataDir(QStringLiteral("share/proj"));
    }
    if (!projDataDir.isEmpty() && QDir(projDataDir).exists()) {
        CPLSetConfigOption("PROJ_DATA", projDataDir.toUtf8().constData());
    }

    CPLSetConfigOption("GDAL_NUM_THREADS", "ALL_CPUS");
    CPLSetConfigOption("GDAL_CACHEMAX", "256");
    CPLSetConfigOption("CPL_DEBUG", "OFF");
    CPLSetConfigOption("CPL_LOG_ERRORS", "ON");
}

}
