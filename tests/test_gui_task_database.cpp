#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QUuid>

#include "task_database.h"
#include "task_manager.h"

namespace {

QCoreApplication* ensureApp() {
    if (auto* existing = QCoreApplication::instance()) {
        return existing;
    }

    static int argc = 1;
    static char arg0[] = "test_gui_task_database";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication app(argc, argv);
    return &app;
}

}  // namespace

TEST(TaskPersistenceTest, StoresLocalizedAndRawResultMessagesSeparately) {
    auto* app = ensureApp();
    ASSERT_NE(app, nullptr);

    const QString displayGroup = QStringLiteral("test_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    auto& manager = gis_ai::gui::TaskManager::instance();
    manager.initializeGroup(displayGroup);

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
        false,
        false,
        QStringLiteral("执行失败"),
        QStringLiteral("onnxruntime: invalid model path"),
        QStringLiteral("D:/tmp/out.tif"));

    const auto record = manager.findTask(displayGroup, taskId);
    EXPECT_EQ(record.resultMessage, QStringLiteral("执行失败"));
    EXPECT_EQ(record.resultRawMessage, QStringLiteral("onnxruntime: invalid model path"));
    EXPECT_EQ(record.outputPath, QStringLiteral("D:/tmp/out.tif"));
    EXPECT_EQ(record.status, gis_ai::gui::TaskRecord::Failed);

    gis_ai::gui::TaskDatabase::instance().closeAll();
}
