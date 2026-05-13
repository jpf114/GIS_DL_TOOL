#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <vector>
#include <map>
#include <string>
#include "param_card_widget.h"
#include "task_database.h"

namespace gis_ai::gui {

class TaskManager : public QObject {
    Q_OBJECT
public:
    static TaskManager& instance();

    void initializeGroup(const QString& displayGroup);

    QString submitTask(const QString& displayGroup,
                       const QString& pluginName,
                       const QString& actionKey,
                       const std::map<std::string, ParamValue>& params,
                       const QString& pluginDisplayName = {},
                       const QString& actionDisplayName = {});
    void updateAndRerunTask(const QString& displayGroup, const QString& id,
                            const std::map<std::string, ParamValue>& newParams);
    void updateTaskStatus(const QString& displayGroup, const QString& id,
                          TaskRecord::Status status);
    void finishTask(const QString& displayGroup, const QString& id,
                    bool success, bool cancelled,
                    const QString& resultMsg, const QString& resultRaw,
                    const QString& outputPath);
    void deleteTasks(const QString& displayGroup, const QStringList& ids);
    void clearHistory(const QString& displayGroup);
    void clearLogsForTask(const QString& displayGroup, const QString& taskId);
    void clearAllLogs(const QString& displayGroup);

    void appendLog(const QString& displayGroup, const QString& taskId,
                   const QString& message, int level = 0);
    QList<TaskLogEntry> logsForTask(const QString& displayGroup,
                                     const QString& taskId) const;

    TaskRecord findTask(const QString& displayGroup, const QString& id) const;
    QList<TaskRecord> recentTasks(const QString& displayGroup, int limit = 200) const;
    int taskCount(const QString& displayGroup) const;

signals:
    void taskSubmitted(const QString& displayGroup, const QString& id);
    void taskStarted(const QString& displayGroup, const QString& id);
    void taskFinished(const QString& displayGroup, const QString& id);
    void taskProgressChanged(const QString& displayGroup, const QString& id, double percent);
    void logAppended(const QString& displayGroup, const QString& taskId,
                     const QString& message);

private:
    TaskManager(QObject* parent = nullptr);
};

}
