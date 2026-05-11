#include "gdal_config.h"

#include <QApplication>
#include <QDir>
#include <gdal_priv.h>
#include <cpl_conv.h>

namespace gis_ai::gui {

void configureGdalRuntime() {
    GDALAllRegister();

    QString gdalDataDir;
    if (qEnvironmentVariableIsSet("GDAL_DATA")) {
        gdalDataDir = QString::fromUtf8(qgetenv("GDAL_DATA"));
    }
    if (gdalDataDir.isEmpty()) {
        QDir appDir(QApplication::applicationDirPath());
        gdalDataDir = appDir.filePath(QStringLiteral("../share/gdal"));
    }
    if (QDir(gdalDataDir).exists()) {
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
        QDir appDir(QApplication::applicationDirPath());
        projDataDir = appDir.filePath(QStringLiteral("../share/proj"));
    }
    if (QDir(projDataDir).exists()) {
        CPLSetConfigOption("PROJ_DATA", projDataDir.toUtf8().constData());
    }

    CPLSetConfigOption("GDAL_NUM_THREADS", "ALL_CPUS");
    CPLSetConfigOption("GDAL_CACHEMAX", "256");
    CPLSetConfigOption("CPL_DEBUG", "OFF");
    CPLSetConfigOption("CPL_LOG_ERRORS", "ON");
}

}
