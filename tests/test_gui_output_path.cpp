#include <gtest/gtest.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>

#include <gdal_priv.h>

#include "mainwindow.h"
#include "task_manager.h"

namespace {

QApplication* ensureApp() {
    if (auto* existing = qobject_cast<QApplication*>(QCoreApplication::instance())) {
        return existing;
    }

    qputenv("QT_QPA_PLATFORM", "offscreen");
    static int argc = 1;
    static char arg0[] = "test_gui_output_path";
    static char* argv[] = {arg0, nullptr};
    static QApplication app(argc, argv);
    return &app;
}

}  // namespace

TEST(GuiOutputPathTest, SuccessfulExecutionPersistsOutputPathToTaskRecord) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    GDALAllRegister();

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    auto& manager = gis_ai::gui::TaskManager::instance();
    manager.initializeGroup(QStringLiteral("segment"));
    manager.clearHistory(QStringLiteral("segment"));

    gis_ai::gui::MainWindow window;
    window.selectPluginByName("segment");
    window.selectActionByKey("segment_raster");

    const QString outputPath = QDir(tempDir.path()).filePath("persisted_output.tif");
    ASSERT_TRUE(window.setParamValue("model_path", "test_data/models/test_seg_model.onnx"));
    ASSERT_TRUE(window.setParamValue("input_raster", "test_data/raster/test_100x100.tif"));
    ASSERT_TRUE(window.setParamValue("output_tif", outputPath.toStdString()));

    bool receivedFinishedSignal = false;
    QObject::connect(&window, &gis_ai::gui::MainWindow::executionFinished, app,
                     [&](bool success) {
                         receivedFinishedSignal = success;
                         app->quit();
                     });

    QTimer::singleShot(0, app, [&window]() {
        window.triggerExecute();
    });
    QTimer::singleShot(10000, app, [&]() {
        app->quit();
    });

    app->exec();

    ASSERT_TRUE(receivedFinishedSignal);
    ASSERT_TRUE(QFileInfo::exists(outputPath));

    const auto records = manager.recentTasks(QStringLiteral("segment"), 1);
    ASSERT_EQ(records.size(), 1);
    EXPECT_EQ(records.front().status, gis_ai::gui::TaskRecord::Completed);
    EXPECT_EQ(records.front().outputPath, outputPath);
}
