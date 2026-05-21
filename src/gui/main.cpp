#include "mainwindow.h"

#include <gis_ai/version.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSaveFile>
#include <QTimer>

#include <gdal_priv.h>
#include <cpl_conv.h>

#include <csignal>
#include <optional>
#include <string>
#include <vector>

namespace {

void initGdalRuntime(const QString& applicationDirPath) {
    GDALAllRegister();

    QString gdalDataDir;
    if (qEnvironmentVariableIsSet("GDAL_DATA")) {
        gdalDataDir = QString::fromUtf8(qgetenv("GDAL_DATA"));
    }
    if (gdalDataDir.isEmpty()) {
        QDir appDir(applicationDirPath);
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
        QDir appDir(applicationDirPath);
        projDataDir = appDir.filePath(QStringLiteral("../share/proj"));
    }
    if (QDir(projDataDir).exists()) {
        CPLSetConfigOption("PROJ_DATA", projDataDir.toUtf8().constData());
    }

    CPLSetConfigOption("CPL_DEBUG", "OFF");
}

void crashSignalHandler(int signal) {
    const char* sigName = "UNKNOWN";
    switch (signal) {
        case SIGSEGV: sigName = "SIGSEGV (段错误)"; break;
        case SIGABRT: sigName = "SIGABRT (异常终止)"; break;
        case SIGFPE:  sigName = "SIGFPE (浮点异常)"; break;
        case SIGILL:  sigName = "SIGILL (非法指令)"; break;
        default: break;
    }
    fprintf(stderr, "GIS AI Tool 遇到致命错误 (%s)，程序即将退出。\n", sigName);
    _exit(signal);
}

}

int main(int argc, char* argv[])
{
    std::signal(SIGSEGV, crashSignalHandler);
    std::signal(SIGABRT, crashSignalHandler);
    std::signal(SIGFPE,  crashSignalHandler);
    std::signal(SIGILL,  crashSignalHandler);

    std::set_terminate([]() {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception& e) {
            fprintf(stderr, "GIS AI Tool 未捕获异常: %s\n", e.what());
        } catch (...) {
            fprintf(stderr, "GIS AI Tool 未捕获未知异常\n");
        }
        _exit(1);
    });

    const QString applicationDirPath = QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath();

    initGdalRuntime(applicationDirPath);

    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("GIS AI TOOLKIT"));
    app.setOrganizationName(QStringLiteral("GIS_AI"));
    app.setApplicationVersion(QString::fromLatin1(GIS_AI_VERSION_STRING));

    {
        QStringList fontSearchDirs;
#ifdef Q_OS_WIN
        const QString winDir = qEnvironmentVariable("SystemRoot");
        if (!winDir.isEmpty()) {
            fontSearchDirs.append(QDir(winDir).filePath(QStringLiteral("Fonts")));
        } else {
            fontSearchDirs.append(QStringLiteral("C:/Windows/Fonts"));
        }
#elif defined(Q_OS_MAC)
        fontSearchDirs.append(QStringLiteral("/System/Library/Fonts"));
        fontSearchDirs.append(QStringLiteral("/Library/Fonts"));
        fontSearchDirs.append(QDir::homePath() + QStringLiteral("/Library/Fonts"));
#else
        fontSearchDirs.append(QStringLiteral("/usr/share/fonts"));
        fontSearchDirs.append(QStringLiteral("/usr/local/share/fonts"));
        fontSearchDirs.append(QDir::homePath() + QStringLiteral("/.local/share/fonts"));
        fontSearchDirs.append(QDir::homePath() + QStringLiteral("/.fonts"));
#endif

        const QStringList cjkFontFileNames = {
            QStringLiteral("msyh.ttc"), QStringLiteral("msyhbd.ttc"),
            QStringLiteral("msyh.ttf"),
            QStringLiteral("Deng.ttf"),
            QStringLiteral("simhei.ttf"), QStringLiteral("simsun.ttc"),
            QStringLiteral("NotoSansSC-Regular.otf"),
            QStringLiteral("NotoSansCJKsc-Regular.otf"),
            QStringLiteral("SourceHanSansSC-Regular.otf"),
            QStringLiteral("WenQuanYiMicroHei.ttf"),
        };

        for (const QString& dir : fontSearchDirs) {
            for (const QString& name : cjkFontFileNames) {
                const QString path = QDir(dir).filePath(name);
                if (QFile::exists(path)) {
                    QFontDatabase::addApplicationFont(path);
                }
            }
        }

        const QStringList preferredFamilies = {
            QStringLiteral("Microsoft YaHei UI"),
            QStringLiteral("Microsoft YaHei"),
            QStringLiteral("Noto Sans SC"),
            QStringLiteral("Noto Sans CJK SC"),
            QStringLiteral("Source Han Sans SC"),
            QStringLiteral("WenQuanYi Micro Hei"),
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

    const QStringList arguments = QCoreApplication::arguments();
    const bool selfTestMode = arguments.contains(QStringLiteral("--self-test"));
    std::optional<QString> screenshotPath;
    std::optional<QString> statusFilePath;
    std::optional<std::string> selectedPlugin;
    std::optional<std::string> selectedAction;
    std::vector<std::pair<std::string, std::string>> paramAssignments;
    std::vector<std::string> failedParamAssignments;
    const bool autoExecute = arguments.contains(QStringLiteral("--auto-execute"));
    const bool quitOnFinish = arguments.contains(QStringLiteral("--quit-on-finish"));

    for (int i = 1; i < arguments.size(); ++i) {
        const QString arg = arguments.at(i);
        if (arg == QStringLiteral("--screenshot") && (i + 1) < arguments.size()) {
            screenshotPath = arguments.at(i + 1);
            ++i;
            continue;
        }
        if (arg == QStringLiteral("--status-file") && (i + 1) < arguments.size()) {
            statusFilePath = arguments.at(i + 1);
            ++i;
            continue;
        }
        if (arg == QStringLiteral("--select-plugin") && (i + 1) < arguments.size()) {
            selectedPlugin = arguments.at(i + 1).toUtf8().toStdString();
            ++i;
            continue;
        }
        if (arg == QStringLiteral("--select-action") && (i + 1) < arguments.size()) {
            selectedAction = arguments.at(i + 1).toUtf8().toStdString();
            ++i;
            continue;
        }
        if (arg == QStringLiteral("--set-param") && (i + 1) < arguments.size()) {
            const QString assignment = arguments.at(i + 1);
            const int pos = assignment.indexOf('=');
            if (pos >= 0) {
                paramAssignments.emplace_back(
                    assignment.left(pos).toUtf8().toStdString(),
                    assignment.mid(pos + 1).toUtf8().toStdString());
            }
            ++i;
        }
    }

    gis_ai::gui::MainWindow window;
    {
        QDir iconDir(applicationDirPath);
        QString iconPath;
        for (int i = 0; i < 6; ++i) {
            QString candidate = iconDir.absoluteFilePath(QStringLiteral("share/icons/bold/brain-bold.svg"));
            if (QFile::exists(candidate)) {
                iconPath = candidate;
                break;
            }
            if (!iconDir.cdUp()) break;
        }
        if (iconPath.isEmpty()) {
            QDir devDir(applicationDirPath);
            if (devDir.cdUp()) {
                QString candidate = devDir.absoluteFilePath(QStringLiteral("resources/icons/bold/brain-bold.svg"));
                if (QFile::exists(candidate)) {
                    iconPath = candidate;
                }
            }
        }
        if (!iconPath.isEmpty()) {
            window.setWindowIcon(QIcon(iconPath));
        }
    }
    if (selectedPlugin.has_value()) {
        window.selectPluginByName(selectedPlugin.value());
    }
    if (selectedAction.has_value()) {
        window.selectActionByKey(selectedAction.value());
    }
    for (const auto& [key, value] : paramAssignments) {
        if (!window.setParamValue(key, value)) {
            failedParamAssignments.push_back(key);
        }
    }
    window.show();

    if (autoExecute && !failedParamAssignments.empty()) {
        QTimer::singleShot(100, &app, [&app, &window, screenshotPath, statusFilePath, failedParamAssignments]() {
            const QString failedKeys = QString::fromUtf8([&failedParamAssignments]() {
                std::string joined;
                for (size_t i = 0; i < failedParamAssignments.size(); ++i) {
                    if (i > 0) {
                        joined += ",";
                    }
                    joined += failedParamAssignments[i];
                }
                return joined;
            }().c_str());

            if (statusFilePath.has_value()) {
                const QFileInfo info(statusFilePath.value());
                if (!info.absoluteDir().exists()) {
                    info.absoluteDir().mkpath(QStringLiteral("."));
                }

                QJsonObject status;
                status.insert(QStringLiteral("success"), false);
                status.insert(QStringLiteral("cancelled"), false);
                status.insert(QStringLiteral("message"),
                    QStringLiteral("以下参数未能应用到当前界面：%1").arg(failedKeys));
                status.insert(QStringLiteral("raw_message"),
                    QStringLiteral("failed_to_apply_param:%1").arg(failedKeys));

                QSaveFile statusFile(statusFilePath.value());
                if (statusFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    statusFile.write(QJsonDocument(status).toJson(QJsonDocument::Indented));
                    statusFile.commit();
                }
            }

            if (screenshotPath.has_value()) {
                const QFileInfo info(screenshotPath.value());
                if (!info.absoluteDir().exists()) {
                    info.absoluteDir().mkpath(QStringLiteral("."));
                }
                window.grab().save(screenshotPath.value());
            }
            app.exit(2);
        });
    }

    if (screenshotPath.has_value() && !autoExecute) {
        const QString targetPath = screenshotPath.value();
        QTimer::singleShot(500, &app, [&app, &window, targetPath]() {
            const QFileInfo info(targetPath);
            if (!info.absoluteDir().exists()) {
                info.absoluteDir().mkpath(QStringLiteral("."));
            }
            window.grab().save(targetPath);
            app.quit();
        });
    }

    if (autoExecute) {
        QTimer::singleShot(200, &app, [&window]() {
            window.triggerExecute();
        });
    }

    if (autoExecute && (quitOnFinish || screenshotPath.has_value() || statusFilePath.has_value())) {
        QObject::connect(&window, &gis_ai::gui::MainWindow::executionFinished, &app,
            [&app, &window, screenshotPath, statusFilePath, quitOnFinish](bool) {
                if (statusFilePath.has_value()) {
                    const QFileInfo info(statusFilePath.value());
                    if (!info.absoluteDir().exists()) {
                        info.absoluteDir().mkpath(QStringLiteral("."));
                    }

                    QJsonObject status;
                    status.insert(QStringLiteral("success"), window.lastExecutionSuccess());
                    status.insert(QStringLiteral("cancelled"), window.lastExecutionCancelled());
                    status.insert(QStringLiteral("message"), window.lastExecutionMessage());
                    status.insert(QStringLiteral("raw_message"), window.lastExecutionRawMessage());

                    QSaveFile statusFile(statusFilePath.value());
                    if (statusFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        statusFile.write(QJsonDocument(status).toJson(QJsonDocument::Indented));
                        statusFile.commit();
                    }
                }
                if (screenshotPath.has_value()) {
                    const QFileInfo info(screenshotPath.value());
                    if (!info.absoluteDir().exists()) {
                        info.absoluteDir().mkpath(QStringLiteral("."));
                    }
                    window.grab().save(screenshotPath.value());
                }
                if (quitOnFinish || screenshotPath.has_value() || statusFilePath.has_value()) {
                    QTimer::singleShot(150, &app, &QApplication::quit);
                }
            });
    }

    if (selfTestMode) {
        QTimer::singleShot(300, &app, &QApplication::quit);
    }

    return app.exec();
}
