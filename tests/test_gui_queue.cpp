#include <gtest/gtest.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QTextEdit>
#include <QTreeWidget>
#include <QMetaObject>
#include <QPushButton>
#include <QThread>
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

bool spinUntil(const std::function<bool()>& predicate,
               int timeoutMs = 6000,
               int intervalMs = 10) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (predicate()) {
            return true;
        }
        QCoreApplication::processEvents();
        QThread::msleep(intervalMs);
    }
    QCoreApplication::processEvents();
    return predicate();
}

QPushButton* findExecuteButton(gis_ai::gui::MainWindow& window) {
    for (auto* button : window.findChildren<QPushButton*>()) {
        if (button && button->text().contains(QStringLiteral("执行处理"))) {
            return button;
        }
    }
    return nullptr;
}

}  // namespace

TEST(GuiQueueTest, BusinessActionSuccessUpdatesExecutionStateAndOutputFile) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    GDALAllRegister();

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    gis_ai::gui::TaskManager::instance().initializeGroup(QStringLiteral("segment"));
    gis_ai::gui::TaskManager::instance().clearHistory(QStringLiteral("segment"));

    gis_ai::gui::MainWindow window;
    window.selectPluginByName("segment");
    window.selectActionByKey("segment_raster");

    const QString outputPath = QDir(tempDir.path()).filePath("single_success_output.tif");
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

    auto* summaryLabel = window.findChild<QLabel*>(QStringLiteral("execSummary"));
    ASSERT_NE(summaryLabel, nullptr);

    EXPECT_TRUE(receivedFinishedSignal);
    EXPECT_TRUE(window.lastExecutionSuccess());
    EXPECT_FALSE(window.lastExecutionCancelled());
    EXPECT_FALSE(window.lastExecutionMessage().isEmpty());
    EXPECT_FALSE(window.lastExecutionRawMessage().isEmpty());
    EXPECT_TRUE(QFileInfo::exists(outputPath));
    EXPECT_NE(summaryLabel->text().indexOf(QStringLiteral("执行成功")), -1);
}

TEST(GuiQueueTest, InvalidModelPathDisablesExecuteButtonBeforeSubmission) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    gis_ai::gui::MainWindow window;
    window.selectPluginByName("segment");
    window.selectActionByKey("segment_raster");

    ASSERT_TRUE(window.setParamValue("model_path", "test_data/models/missing_model.onnx"));
    ASSERT_TRUE(window.setParamValue("input_raster", "test_data/raster/test_100x100.tif"));
    ASSERT_TRUE(window.setParamValue(
        "output_tif",
        QDir(tempDir.path()).filePath("invalid_model_output.tif").toStdString()));
    QCoreApplication::processEvents();

    QPushButton* executeButton = findExecuteButton(window);
    ASSERT_NE(executeButton, nullptr);

    EXPECT_FALSE(executeButton->isEnabled());
    EXPECT_FALSE(executeButton->toolTip().isEmpty());
    EXPECT_FALSE(gis_ai::gui::TaskRunner::instance().isRunning());
}

TEST(GuiQueueTest, BatchExecutionProcessesMatchingRasterFiles) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    GDALAllRegister();

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    const QDir rootDir(tempDir.path());
    const QString batchDirPath = rootDir.filePath("batch_inputs");
    ASSERT_TRUE(rootDir.mkpath(QStringLiteral("batch_inputs")));
    ASSERT_TRUE(QFile::copy("test_data/raster/test_100x100.tif",
                            QDir(batchDirPath).filePath("tile_a.tif")));
    ASSERT_TRUE(QFile::copy("test_data/raster/test_100x100.tif",
                            QDir(batchDirPath).filePath("tile_b.tif")));

    auto& manager = gis_ai::gui::TaskManager::instance();
    manager.initializeGroup(QStringLiteral("segment"));
    manager.clearHistory(QStringLiteral("segment"));

    gis_ai::gui::MainWindow window;
    window.selectPluginByName("segment");
    window.selectActionByKey("segment_raster");

    ASSERT_TRUE(window.setParamValue("model_path", "test_data/models/test_seg_model.onnx"));
    ASSERT_TRUE(window.setParamValue("input_raster", "test_data/raster/test_100x100.tif"));
    ASSERT_TRUE(window.setParamValue("output_tif",
        QDir(tempDir.path()).filePath("placeholder_output.tif").toStdString()));
    ASSERT_TRUE(window.setBatchMode(true));
    ASSERT_TRUE(window.setBatchInputDirectory(batchDirPath.toStdString()));
    ASSERT_TRUE(window.setBatchFilter("*.tif"));

    window.triggerExecute();

    const bool completed = spinUntil([]() {
        return !gis_ai::gui::TaskRunner::instance().isRunning() &&
               gis_ai::gui::TaskRunner::instance().queuedCount() == 0;
    }, 10000);

    ASSERT_TRUE(completed);

    const auto records = manager.recentTasks(QStringLiteral("segment"), 2);
    ASSERT_EQ(records.size(), 2);
    for (const auto& record : records) {
        EXPECT_EQ(record.status, gis_ai::gui::TaskRecord::Completed);
        EXPECT_TRUE(record.success);
    }

    EXPECT_TRUE(QFileInfo::exists(QDir(batchDirPath).filePath("tile_a_segment_raster.tif")));
    EXPECT_TRUE(QFileInfo::exists(QDir(batchDirPath).filePath("tile_b_segment_raster.tif")));
    EXPECT_TRUE(window.lastExecutionSuccess());
}

TEST(GuiQueueTest, SecondExecuteRequestQueuesWhileFirstTaskIsRunning) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    GDALAllRegister();

    gis_ai::gui::TaskRunner::setTaskStartDelayForTesting(200);

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
    auto* summaryLabel = window.findChild<QLabel*>(QStringLiteral("execSummary"));
    ASSERT_NE(summaryLabel, nullptr);

    bool secondSubmissionAttempted = false;
    bool inspectedQueue = false;
    bool sawQueuedState = false;
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
                sawQueuedState =
                    observedQueuedCount == 1 ||
                    summaryLabel->text().contains(QStringLiteral("队列"));
                const QString runningTaskId = gis_ai::gui::TaskRunner::instance().runningTaskId();
                if (!runningTaskId.isEmpty()) {
                    gis_ai::gui::TaskRunner::instance().cancelTask(runningTaskId);
                }
                waitForRunnerToStop();
            });
            return;
        }

        if (++retryCount > 500) {
            app->quit();
            return;
        }

        QTimer::singleShot(1, app, trySubmitSecond);
    };

    QTimer::singleShot(0, app, [&window]() {
        window.triggerExecute();
    });
    QTimer::singleShot(0, app, trySubmitSecond);
    QTimer::singleShot(8000, app, [&]() {
        if (!inspectedQueue) {
            app->quit();
        }
    });

    app->exec();

    gis_ai::gui::TaskRunner::resetTaskStartDelayForTesting();

    EXPECT_TRUE(secondSubmissionAttempted);
    EXPECT_TRUE(inspectedQueue);
    EXPECT_TRUE(sawQueuedState);
}

TEST(GuiQueueTest, QueuedTaskSummarySwitchesToRunningWhenExecutionStarts) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    GDALAllRegister();

    gis_ai::gui::TaskRunner::setTaskStartDelayForTesting(200);

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    gis_ai::gui::MainWindow window;
    window.selectPluginByName("segment");
    window.selectActionByKey("segment_raster");

    ASSERT_TRUE(window.setParamValue("model_path", "test_data/models/test_seg_model.onnx"));
    ASSERT_TRUE(window.setParamValue("input_raster", "test_data/raster/test_100x100.tif"));
    ASSERT_TRUE(window.setParamValue(
        "output_tif",
        QDir(tempDir.path()).filePath("queue_status_output.tif").toStdString()));

    auto* summaryLabel = window.findChild<QLabel*>(QStringLiteral("execSummary"));
    ASSERT_NE(summaryLabel, nullptr);

    bool sawQueuedSummary = false;
    bool sawActiveSummaryAfterQueue = false;
    int retryCount = 0;
    int startRetryCount = 0;
    int shutdownRetryCount = 0;
    QString firstTaskId;

    std::function<void()> waitForSecondTaskToStart;
    std::function<void()> waitForRunnerToStop;
    std::function<void()> trySubmitSecond;

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

    waitForSecondTaskToStart = [&]() {
        const QString runningTaskId = gis_ai::gui::TaskRunner::instance().runningTaskId();
        const QString summary = summaryLabel->text();
        if (!runningTaskId.isEmpty() &&
            runningTaskId != firstTaskId &&
            gis_ai::gui::TaskRunner::instance().queuedCount() == 0) {
            sawActiveSummaryAfterQueue =
                !summary.contains(QStringLiteral("队列")) &&
                (summary.contains(QStringLiteral("正在执行")) ||
                 summary.contains(QStringLiteral("执行成功")) ||
                 summary.contains(QStringLiteral("已取消")) ||
                 summary.contains(QStringLiteral("执行失败")));
            gis_ai::gui::TaskRunner::instance().cancelTask(runningTaskId);
            waitForRunnerToStop();
            return;
        }

        if (++startRetryCount > 300) {
            app->quit();
            return;
        }

        QTimer::singleShot(10, app, waitForSecondTaskToStart);
    };

    trySubmitSecond = [&]() {
        if (gis_ai::gui::TaskRunner::instance().isRunning()) {
            firstTaskId = gis_ai::gui::TaskRunner::instance().runningTaskId();
            window.triggerExecute();
            sawQueuedSummary = summaryLabel->text().contains(QStringLiteral("队列"));
            if (!firstTaskId.isEmpty()) {
                gis_ai::gui::TaskRunner::instance().cancelTask(firstTaskId);
            }
            QTimer::singleShot(10, app, waitForSecondTaskToStart);
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
    QTimer::singleShot(8000, app, [&]() {
        app->quit();
    });

    app->exec();

    gis_ai::gui::TaskRunner::resetTaskStartDelayForTesting();

    EXPECT_TRUE(sawQueuedSummary);
    EXPECT_TRUE(sawActiveSummaryAfterQueue);
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

TEST(GuiQueueTest, SelectedTaskResultPanelRefreshesWhenTaskFinishes) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    gis_ai::gui::MainWindow window;
    auto* taskCenterPage = window.findChild<gis_ai::gui::TaskCenterPage*>();
    ASSERT_NE(taskCenterPage, nullptr);

    auto& manager = gis_ai::gui::TaskManager::instance();
    const QString displayGroup = QStringLiteral("queue_live_result_test");
    manager.initializeGroup(displayGroup);
    manager.clearHistory(displayGroup);

    taskCenterPage->setCurrentGroup(displayGroup);

    const QString taskId = manager.submitTask(
        displayGroup,
        QStringLiteral("segment"),
        QStringLiteral("segment_raster"),
        {},
        QStringLiteral("图像分割"),
        QStringLiteral("单幅分割"));
    ASSERT_FALSE(taskId.isEmpty());

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

    taskTree->setCurrentItem(targetItem);
    QCoreApplication::processEvents();

    const QString beforeText = resultDisplay->toPlainText();
    EXPECT_NE(beforeText.indexOf(QStringLiteral("等待中")), -1);

    manager.finishTask(
        displayGroup,
        taskId,
        true,
        false,
        QStringLiteral("执行成功"),
        QStringLiteral("tile written successfully"),
        QStringLiteral("D:/tmp/finished.tif"));
    QCoreApplication::processEvents();

    const QString afterText = resultDisplay->toPlainText();
    EXPECT_NE(afterText.indexOf(QStringLiteral("执行成功")), -1);
    EXPECT_NE(afterText.indexOf(QStringLiteral("tile written successfully")), -1);
    EXPECT_NE(afterText.indexOf(QStringLiteral("D:/tmp/finished.tif")), -1);
}

TEST(GuiQueueTest, RerunRequestReloadsHistoryContextIntoMainPanel) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    gis_ai::gui::MainWindow window;
    auto* taskCenterPage = window.findChild<gis_ai::gui::TaskCenterPage*>();
    ASSERT_NE(taskCenterPage, nullptr);

    auto& manager = gis_ai::gui::TaskManager::instance();
    const QString displayGroup = QStringLiteral("segment");
    manager.initializeGroup(displayGroup);
    manager.clearHistory(displayGroup);

    std::map<std::string, ParamValue> params;
    params["model_path"] = std::string("test_data/models/test_seg_model.onnx");
    params["input_raster"] = std::string("test_data/raster/test_100x100.tif");
    params["output_tif"] = std::string("D:/tmp/rerun_out.tif");
    const QString taskId = manager.submitTask(
        displayGroup,
        QStringLiteral("segment"),
        QStringLiteral("segment_raster"),
        params,
        QStringLiteral("大图分割"),
        QStringLiteral("仅输出栅格"));
    ASSERT_FALSE(taskId.isEmpty());

    manager.finishTask(
        displayGroup,
        taskId,
        true,
        false,
        QStringLiteral("执行成功"),
        QStringLiteral("rerun raw message"),
        QStringLiteral("D:/tmp/rerun_out.tif"));

    taskCenterPage->setCurrentGroup(displayGroup);

    const bool invoked = QMetaObject::invokeMethod(
        taskCenterPage,
        "rerunTaskRequested",
        Qt::DirectConnection,
        Q_ARG(QString, taskId));
    ASSERT_TRUE(invoked);
    QCoreApplication::processEvents();

    auto* summaryLabel = window.findChild<QLabel*>(QStringLiteral("execSummary"));
    ASSERT_NE(summaryLabel, nullptr);
    EXPECT_NE(summaryLabel->text().indexOf(taskId), -1);
    EXPECT_NE(summaryLabel->text().indexOf(QStringLiteral("历史参数")), -1);

    auto* statusLabel = window.findChild<QLabel*>(QStringLiteral("statusBarLabel"));
    ASSERT_NE(statusLabel, nullptr);
    EXPECT_NE(statusLabel->text().indexOf(QStringLiteral("仅输出栅格")), -1);

    EXPECT_TRUE(window.setParamValue("output_tif", "D:/tmp/rerun_out_changed.tif"));
}

TEST(GuiQueueTest, ClearHistoryResetsExecutionSummary) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    gis_ai::gui::MainWindow window;
    auto* taskCenterPage = window.findChild<gis_ai::gui::TaskCenterPage*>();
    auto* summaryLabel = window.findChild<QLabel*>(QStringLiteral("execSummary"));
    ASSERT_NE(taskCenterPage, nullptr);
    ASSERT_NE(summaryLabel, nullptr);

    auto& manager = gis_ai::gui::TaskManager::instance();
    const QString displayGroup = QStringLiteral("segment");
    manager.initializeGroup(displayGroup);
    manager.clearHistory(displayGroup);

    auto allRecords = manager.recentTasks(displayGroup, 1000);
    if (!allRecords.empty()) {
        QStringList allIds;
        for (const auto& rec : allRecords) {
            allIds.append(rec.id);
        }
        manager.deleteTasks(displayGroup, allIds);
    }

    const QString taskId = manager.submitTask(
        displayGroup,
        QStringLiteral("segment"),
        QStringLiteral("segment_raster"),
        {},
        QStringLiteral("大图分割"),
        QStringLiteral("仅输出栅格"));
    ASSERT_FALSE(taskId.isEmpty());

    manager.finishTask(
        displayGroup,
        taskId,
        true,
        false,
        QStringLiteral("执行成功"),
        QStringLiteral("clear history raw"),
        QStringLiteral("D:/tmp/clear_history_out.tif"));

    taskCenterPage->setCurrentGroup(QString());
    taskCenterPage->setCurrentGroup(displayGroup);
    summaryLabel->setText(QStringLiteral("任务 %1 执行成功。执行成功").arg(taskId));

    const bool invoked = QMetaObject::invokeMethod(
        taskCenterPage,
        "clearHistoryRequested",
        Qt::DirectConnection);
    ASSERT_TRUE(invoked);
    QCoreApplication::processEvents();

    EXPECT_NE(summaryLabel->text().indexOf(QStringLiteral("当前未执行任务")), -1);
}

TEST(GuiQueueTest, DeleteLastTaskResetsExecutionSummary) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    gis_ai::gui::MainWindow window;
    auto* taskCenterPage = window.findChild<gis_ai::gui::TaskCenterPage*>();
    auto* summaryLabel = window.findChild<QLabel*>(QStringLiteral("execSummary"));
    ASSERT_NE(taskCenterPage, nullptr);
    ASSERT_NE(summaryLabel, nullptr);

    auto& manager = gis_ai::gui::TaskManager::instance();
    const QString displayGroup = QStringLiteral("segment");
    manager.initializeGroup(displayGroup);
    manager.clearHistory(displayGroup);

    const QString taskId = manager.submitTask(
        displayGroup,
        QStringLiteral("segment"),
        QStringLiteral("segment_raster"),
        {},
        QStringLiteral("大图分割"),
        QStringLiteral("仅输出栅格"));
    ASSERT_FALSE(taskId.isEmpty());

    manager.finishTask(
        displayGroup,
        taskId,
        true,
        false,
        QStringLiteral("执行成功"),
        QStringLiteral("delete last raw"),
        QStringLiteral("D:/tmp/delete_last_out.tif"));

    taskCenterPage->setCurrentGroup(displayGroup);
    summaryLabel->setText(QStringLiteral("任务 %1 执行成功。执行成功").arg(taskId));

    const bool invoked = QMetaObject::invokeMethod(
        taskCenterPage,
        "deleteTasksRequested",
        Qt::DirectConnection,
        Q_ARG(QStringList, QStringList{taskId}));
    ASSERT_TRUE(invoked);
    QCoreApplication::processEvents();

    EXPECT_NE(summaryLabel->text().indexOf(QStringLiteral("当前未执行任务")), -1);
}

TEST(GuiQueueTest, SwitchingGroupClearsStaleExecutionSummary) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    gis_ai::gui::MainWindow window;
    auto* summaryLabel = window.findChild<QLabel*>(QStringLiteral("execSummary"));
    auto* statusLabel = window.findChild<QLabel*>(QStringLiteral("statusBarLabel"));
    ASSERT_NE(summaryLabel, nullptr);
    ASSERT_NE(statusLabel, nullptr);

    window.selectPluginByName("segment");
    window.selectActionByKey("segment_raster");
    summaryLabel->setText(QStringLiteral("任务 task-segment 执行成功。执行成功"));

    window.selectPluginByName("inference");
    QCoreApplication::processEvents();

    EXPECT_TRUE(
        summaryLabel->text().contains(QStringLiteral("请选择该主功能下的子功能")) ||
        summaryLabel->text().contains(QStringLiteral("当前未执行任务")));
    EXPECT_NE(statusLabel->text().indexOf(QStringLiteral("模型推理")), -1);
}

TEST(GuiQueueTest, BackgroundTaskFinishDoesNotOverwriteDifferentGroupSummary) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    GDALAllRegister();

    gis_ai::gui::TaskRunner::setTaskStartDelayForTesting(200);

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());
    const QDir rootDir(tempDir.path());
    const QString batchDirPath = rootDir.filePath("background_batch_inputs");
    ASSERT_TRUE(rootDir.mkpath(QStringLiteral("background_batch_inputs")));
    ASSERT_TRUE(QFile::copy("test_data/raster/test_100x100.tif",
                            QDir(batchDirPath).filePath("tile_a.tif")));
    ASSERT_TRUE(QFile::copy("test_data/raster/test_100x100.tif",
                            QDir(batchDirPath).filePath("tile_b.tif")));

    gis_ai::gui::MainWindow window;
    auto* summaryLabel = window.findChild<QLabel*>(QStringLiteral("execSummary"));
    auto* statusLabel = window.findChild<QLabel*>(QStringLiteral("statusBarLabel"));
    ASSERT_NE(summaryLabel, nullptr);
    ASSERT_NE(statusLabel, nullptr);

    window.selectPluginByName("segment");
    window.selectActionByKey("segment_raster");

    ASSERT_TRUE(window.setParamValue("model_path", "test_data/models/test_seg_model.onnx"));
    ASSERT_TRUE(window.setParamValue("input_raster", "test_data/raster/test_100x100.tif"));
    ASSERT_TRUE(window.setParamValue(
        "output_tif",
        QDir(tempDir.path()).filePath("background_finish_output.tif").toStdString()));
    ASSERT_TRUE(window.setBatchMode(true));
    ASSERT_TRUE(window.setBatchInputDirectory(batchDirPath.toStdString()));
    ASSERT_TRUE(window.setBatchFilter("*.tif"));

    bool switchedGroupWhileRunning = false;
    bool summaryStayedOnCurrentGroup = false;
    int retryCount = 0;

    std::function<void()> waitForTaskFinish;
    std::function<void()> switchWhenRunning;

    waitForTaskFinish = [&]() {
        if (!gis_ai::gui::TaskRunner::instance().isRunning()) {
            const QString summary = summaryLabel->text();
            summaryStayedOnCurrentGroup =
                !summary.contains(QStringLiteral("执行成功")) &&
                !summary.contains(QStringLiteral("执行失败")) &&
                !summary.contains(QStringLiteral("已取消")) &&
                (summary.contains(QStringLiteral("请选择该主功能下的子功能")) ||
                 summary.contains(QStringLiteral("当前未执行任务")));
            app->quit();
            return;
        }

        if (++retryCount > 300) {
            app->quit();
            return;
        }

        QTimer::singleShot(10, app, waitForTaskFinish);
    };

    switchWhenRunning = [&]() {
        if (gis_ai::gui::TaskRunner::instance().isRunning()) {
            window.selectPluginByName("inference");
            switchedGroupWhileRunning = true;
            QTimer::singleShot(10, app, waitForTaskFinish);
            return;
        }

        if (++retryCount > 300) {
            app->quit();
            return;
        }

        QTimer::singleShot(5, app, switchWhenRunning);
    };

    QTimer::singleShot(0, app, [&window]() {
        window.triggerExecute();
    });
    QTimer::singleShot(0, app, switchWhenRunning);
    QTimer::singleShot(8000, app, [&]() {
        app->quit();
    });

    app->exec();

    gis_ai::gui::TaskRunner::resetTaskStartDelayForTesting();

    EXPECT_TRUE(switchedGroupWhileRunning);
    EXPECT_TRUE(summaryStayedOnCurrentGroup);
    EXPECT_NE(statusLabel->text().indexOf(QStringLiteral("模型推理")), -1);
}

TEST(GuiQueueTest, PluginSelectionWithoutActionKeepsPanelStateConsistent) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    gis_ai::gui::MainWindow window;
    auto* titleLabel = window.findChild<QLabel*>(QStringLiteral("heroTitle"));
    auto* descLabel = window.findChild<QLabel*>(QStringLiteral("heroDesc"));
    auto* metaLabel = window.findChild<QLabel*>(QStringLiteral("mainHeroMeta"));
    auto* summaryLabel = window.findChild<QLabel*>(QStringLiteral("execSummary"));
    auto* statusLabel = window.findChild<QLabel*>(QStringLiteral("statusBarLabel"));
    auto* taskCenterPage = window.findChild<gis_ai::gui::TaskCenterPage*>();
    QPushButton* executeButton = nullptr;
    for (auto* button : window.findChildren<QPushButton*>()) {
        if (button && button->text().contains(QStringLiteral("执行处理"))) {
            executeButton = button;
            break;
        }
    }
    ASSERT_NE(titleLabel, nullptr);
    ASSERT_NE(descLabel, nullptr);
    ASSERT_NE(metaLabel, nullptr);
    ASSERT_NE(summaryLabel, nullptr);
    ASSERT_NE(statusLabel, nullptr);
    ASSERT_NE(taskCenterPage, nullptr);
    ASSERT_NE(executeButton, nullptr);

    summaryLabel->setText(QStringLiteral("任务 stale-task 执行成功。执行成功"));

    window.selectPluginByName("segment");
    QCoreApplication::processEvents();

    EXPECT_NE(titleLabel->text().indexOf(QStringLiteral("大图分割")), -1);
    EXPECT_NE(descLabel->text().indexOf(QStringLiteral("请选择子功能")), -1);
    EXPECT_NE(metaLabel->text().indexOf(QStringLiteral("已选择主功能")), -1);
    EXPECT_NE(statusLabel->text().indexOf(QStringLiteral("大图分割")), -1);
    EXPECT_NE(summaryLabel->text().indexOf(QStringLiteral("请选择该主功能下的子功能")), -1);
    EXPECT_EQ(taskCenterPage->currentGroup(), QStringLiteral("segment"));
    EXPECT_FALSE(executeButton->isEnabled());
}
