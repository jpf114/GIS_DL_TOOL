#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <string>

namespace gis_ai::gui {

class ProgressDialog;

class ExecutionController : public QObject {
    Q_OBJECT
public:
    explicit ExecutionController(QObject* parent = nullptr);

    void setDisplayGroupKey(const std::string& key);
    const std::string& displayGroupKey() const;

    void trackTask(const QString& taskId, const QString& group, bool wasRunning);

    bool hasPendingExecution() const;
    int pendingCountForCurrentGroup() const;
    void discardTaskState(const QStringList& taskIds);

    QString currentTaskId() const;
    bool isTaskTracked(const QString& taskId) const;
    QString taskGroup(const QString& taskId) const;

    bool lastSuccess() const;
    bool lastCancelled() const;
    QString lastMessage() const;
    QString lastRawMessage() const;

    ProgressDialog* progressDialog() const;
    void ensureProgressDialog(QWidget* parent);
    void destroyProgressDialog();

signals:
    void executionFinished(bool success);
    void activeTaskStarted(const QString& taskId, bool affectsCurrentGroup);
    void taskCompleted(const QString& taskId, bool affectsCurrentGroup,
                       bool success, bool cancelled,
                       const QString& message, const QString& rawMessage,
                       bool wasCurrentTask);
    void progressChanged(const QString& taskId, bool affectsCurrentGroup, double percent);
    void logMessage(const QString& taskId, bool affectsCurrentGroup, const QString& msg);
    void activeTaskDiscarded();

private slots:
    void onTaskRunnerStarted(const QString& displayGroup, const QString& taskId);
    void onTaskRunnerFinished(const QString& displayGroup, const QString& taskId,
                              bool success, bool cancelled);
    void onTaskProgressChanged(const QString& taskId, double percent);
    void onTaskLogMessage(const QString& displayGroup, const QString& taskId, const QString& msg);

private:
    bool affectsCurrentGroup(const QString& group) const;

    std::string currentDisplayGroupKey_{"default"};
    QString currentTaskId_;
    QMap<QString, QString> pendingResultTaskIds_;
    bool lastExecutionSuccess_ = false;
    bool lastExecutionCancelled_ = false;
    QString lastExecutionMessage_;
    QString lastExecutionRawMessage_;
    ProgressDialog* progressDialog_ = nullptr;
};

}
