#include <gtest/gtest.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QTextEdit>
#include <QTreeWidget>
#include <QMetaObject>
#include <QTemporaryDir>
#include <QTimer>

#include <gdal_priv.h>

#include "mainwindow.h"
#include "task_center_page.h"
#include "task_manager.h"
#include "task_runner.h"

namespace {

QApplication* ensureApp() {
    if (auto* existing = qobject_cast<QApplication*>(QCoreApplication::instance())) {
        return existing;
    }

    qputenv("QT_QPA_PLATFORM", "offscreen");
    static int argc = 1;
    static char arg0[] = "test_gui_queue";
    static char* argv[] = {arg0, nullptr};
    static QApplication app(argc, argv);
    return &app;
}

}  // namespace

TEST(GuiQueueTest, SecondExecuteRequestQueuesWhileFirstTaskIsRunning) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    GDALAllRegister();

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    gis_ai::gui::MainWindow window;
    window.selectPluginByName("segment");
    window.selectActionByKey("segment_raster");

    ASSERT_TRUE(window.setParamValue("model_path", "test_data/models/test_seg_model.onnx"));
    ASSERT_TRUE(window.setParamValue("input_raster", "test_data/raster/test_100x100.tif"));
    ASSERT_TRUE(window.setParamValue(
        "output_tif",
        QDir(tempDir.path()).filePath("queue_output.tif").toStdString()));

    bool secondSubmissionAttempted = false;
    bool inspectedQueue = false;
    int observedQueuedCount = -1;
    int retryCount = 0;
    int shutdownRetryCount = 0;

    std::function<void()> trySubmitSecond;
    std::function<void()> waitForRunnerToStop;
    waitForRunnerToStop = [&]() {
        if (!gis_ai::gui::TaskRunner::instance().isRunning()) {
            app->quit();
            return;
        }

        if (++shutdownRetryCount > 200) {
            app->quit();
            return;
        }

        QTimer::singleShot(10, app, waitForRunnerToStop);
    };

    trySubmitSecond = [&]() {
        if (gis_ai::gui::TaskRunner::instance().isRunning()) {
            secondSubmissionAttempted = true;
            window.triggerExecute();
            QTimer::singleShot(25, app, [&]() {
                observedQueuedCount = gis_ai::gui::TaskRunner::instance().queuedCount();
                inspectedQueue = true;
                const QString runningTaskId = gis_ai::gui::TaskRunner::instance().runningTaskId();
                if (!runningTaskId.isEmpty()) {
                    gis_ai::gui::TaskRunner::instance().cancelTask(runningTaskId);
                }
                waitForRunnerToStop();
            });
            return;
        }

        if (++retryCount > 200) {
            app->quit();
            return;
        }

        QTimer::singleShot(5, app, trySubmitSecond);
    };

    QTimer::singleShot(0, app, [&window]() {
        window.triggerExecute();
    });
    QTimer::singleShot(0, app, trySubmitSecond);
    QTimer::singleShot(5000, app, [&]() {
        if (!inspectedQueue) {
            app->quit();
        }
    });

    app->exec();

    EXPECT_TRUE(secondSubmissionAttempted);
    EXPECT_TRUE(inspectedQueue);
    EXPECT_EQ(observedQueuedCount, 1);
}

TEST(GuiQueueTest, ClearLogsRequestRemovesTaskLogsInsteadOfAppendingMarker) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    gis_ai::gui::MainWindow window;
    auto* taskCenterPage = window.findChild<gis_ai::gui::TaskCenterPage*>();
    ASSERT_NE(taskCenterPage, nullptr);

    auto& manager = gis_ai::gui::TaskManager::instance();
    manager.initializeGroup(QStringLiteral("default"));

    const QString taskId = manager.submitTask(
        QStringLiteral("default"),
        QStringLiteral("segment"),
        QStringLiteral("segment_raster"),
        {},
        QStringLiteral("大图分割"),
        QStringLiteral("仅输出栅格"));
    ASSERT_FALSE(taskId.isEmpty());

    manager.appendLog(QStringLiteral("default"), taskId, QStringLiteral("第一条日志"));
    manager.appendLog(QStringLiteral("default"), taskId, QStringLiteral("第二条日志"));
    ASSERT_EQ(manager.logsForTask(QStringLiteral("default"), taskId).size(), 2);

    const bool invoked = QMetaObject::invokeMethod(
        taskCenterPage,
        "clearLogsRequested",
        Qt::DirectConnection,
        Q_ARG(QString, taskId));
    ASSERT_TRUE(invoked);

    EXPECT_EQ(manager.logsForTask(QStringLiteral("default"), taskId).size(), 0);
}

TEST(GuiQueueTest, FinishedTaskShowsStructuredResultDetails) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    gis_ai::gui::MainWindow window;
    auto* taskCenterPage = window.findChild<gis_ai::gui::TaskCenterPage*>();
    ASSERT_NE(taskCenterPage, nullptr);

    auto& manager = gis_ai::gui::TaskManager::instance();
    const QString displayGroup = QStringLiteral("queue_detail_test");
    manager.initializeGroup(displayGroup);
    manager.clearHistory(displayGroup);

    const QString taskId = manager.submitTask(
        displayGroup,
        QStringLiteral("segment"),
        QStringLiteral("segment_raster"),
        {},
        QStringLiteral("图像分割"),
        QStringLiteral("单幅分割"));
    ASSERT_FALSE(taskId.isEmpty());

    manager.finishTask(
        displayGroup,
        taskId,
        false,
        false,
        QStringLiteral("执行失败"),
        QStringLiteral("onnxruntime: invalid model path"),
        QStringLiteral("D:/tmp/out.tif"));

    taskCenterPage->setCurrentGroup(displayGroup);

    auto* taskTree = taskCenterPage->findChild<QTreeWidget*>();
    ASSERT_NE(taskTree, nullptr);
    ASSERT_GT(taskTree->topLevelItemCount(), 0);

    auto* resultDisplay = taskCenterPage->findChild<QTextEdit*>(QStringLiteral("taskResultDisplay"));
    ASSERT_NE(resultDisplay, nullptr);

    QTreeWidgetItem* targetItem = nullptr;
    for (int i = 0; i < taskTree->topLevelItemCount(); ++i) {
        auto* item = taskTree->topLevelItem(i);
        if (item != nullptr && item->data(0, Qt::UserRole).toString() == taskId) {
            targetItem = item;
            break;
        }
    }
    ASSERT_NE(targetItem, nullptr);

    taskTree->setCurrentItem(nullptr);
    taskTree->setCurrentItem(targetItem);
    QCoreApplication::processEvents();

    const QString detailText = resultDisplay->toPlainText();
    EXPECT_NE(detailText.indexOf(QStringLiteral("执行失败")), -1);
    EXPECT_NE(detailText.indexOf(QStringLiteral("onnxruntime: invalid model path")), -1);
    EXPECT_NE(detailText.indexOf(QStringLiteral("D:/tmp/out.tif")), -1);
    EXPECT_NE(detailText.indexOf(QStringLiteral("耗时")), -1);
}
