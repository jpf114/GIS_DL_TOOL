#pragma once

#include <QObject>
#include <QMap>
#include <QQueue>
#include <QString>

#include <map>
#include <memory>
#include <string>

#include "param_card_widget.h"

namespace gis_ai::gui {

class QtProgressReporter;

class ExecuteWorker;

struct TaskContext {
    QString taskId;
    QString displayGroup;
    std::unique_ptr<QtProgressReporter> reporter;
};

struct QueuedTask {
    QString displayGroup;
    QString pluginName;
    QString actionKey;
    std::map<std::string, ParamValue> params;
    QString pluginDisplayName;
    QString actionDisplayName;
    QString taskId;
};

class TaskRunner : public QObject {
    Q_OBJECT
public:
    static TaskRunner& instance();

    QString run(const QString& displayGroup,
                const QString& pluginName,
                const QString& actionKey,
                const std::map<std::string, ParamValue>& params,
                const QString& pluginDisplayName,
                const QString& actionDisplayName);

    void cancelTask(const QString& taskId);
    bool isRunning() const;
    QString runningTaskId() const;
    int queuedCount() const;

signals:
    void taskStarted(const QString& displayGroup, const QString& taskId);
    void taskFinished(const QString& displayGroup, const QString& taskId,
                      bool success, bool cancelled);
    void taskProgressChanged(const QString& taskId, double percent);
    void taskLogMessage(const QString& displayGroup, const QString& taskId,
                        const QString& msg);
    void queueChanged(int pendingCount);

private:
    explicit TaskRunner(QObject* parent = nullptr);

    void processQueue();
    void startTask(const QueuedTask& task);

    QQueue<QueuedTask> queue_;
    QMap<QString, std::shared_ptr<TaskContext>> activeTasks_;
    QString runningTaskId_;
};

}  // namespace gis_ai::gui
