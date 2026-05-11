#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QTimer>

#include <gdal_priv.h>
#include <cpl_conv.h>

#include <string>

namespace {

void initGdalRuntime() {
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

    CPLSetConfigOption("CPL_DEBUG", "OFF");
}

}

int main(int argc, char* argv[])
{
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("GIS AI TOOLKIT"));
    app.setOrganizationName(QStringLiteral("GIS_AI"));

    {
        const QStringList candidateFontFiles = {
            QStringLiteral("C:/Windows/Fonts/msyh.ttc"),
            QStringLiteral("C:/Windows/Fonts/msyhbd.ttc"),
            QStringLiteral("C:/Windows/Fonts/msyh.ttf"),
            QStringLiteral("C:/Windows/Fonts/Deng.ttf"),
            QStringLiteral("C:/Windows/Fonts/simhei.ttf"),
            QStringLiteral("C:/Windows/Fonts/simsun.ttc")
        };
        for (const QString& fontFile : candidateFontFiles) {
            if (QFile::exists(fontFile)) {
                QFontDatabase::addApplicationFont(fontFile);
            }
        }

        const QStringList preferredFamilies = {
            QStringLiteral("Microsoft YaHei UI"),
            QStringLiteral("Microsoft YaHei"),
            QStringLiteral("Noto Sans SC"),
            QStringLiteral("Noto Sans CJK SC"),
            QStringLiteral("Source Han Sans SC"),
            QStringLiteral("DengXian"),
            QStringLiteral("SimHei"),
            QStringLiteral("SimSun")
        };
        const QFontDatabase fontDb;
        QFont uiFont = app.font();
        for (const QString& family : preferredFamilies) {
            if (fontDb.families().contains(family)) {
                uiFont.setFamily(family);
                uiFont.setPointSize(10);
                uiFont.setStyleStrategy(QFont::PreferAntialias);
                uiFont.setHintingPreference(QFont::PreferFullHinting);
                app.setFont(uiFont);
                app.setStyleSheet(QStringLiteral("* { font-family: \"%1\"; }").arg(family));
                break;
            }
        }
    }

    initGdalRuntime();

    const QStringList arguments = QCoreApplication::arguments();
    const bool selfTestMode = arguments.contains(QStringLiteral("--self-test"));

    gis_ai::gui::MainWindow window;
    window.show();

    if (selfTestMode) {
        QTimer::singleShot(300, &app, &QApplication::quit);
    }

    return app.exec();
}
