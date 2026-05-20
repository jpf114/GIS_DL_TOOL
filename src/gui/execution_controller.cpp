#include "execution_controller.h"
#include "progress_dialog.h"
#include "task_runner.h"
#include "task_manager.h"

namespace gis_ai::gui {

ExecutionController::ExecutionController(QObject* parent)
    : QObject(parent) {
    connect(&TaskRunner::instance(), &TaskRunner::taskStarted,
            this, &ExecutionController::onTaskRunnerStarted);
    connect(&TaskRunner::instance(), &TaskRunner::taskFinished,
            this, &ExecutionController::onTaskRunnerFinished);
    connect(&TaskRunner::instance(), &TaskRunner::taskProgressChanged,
            this, &ExecutionController::onTaskProgressChanged);
    connect(&TaskRunner::instance(), &TaskRunner::taskLogMessage,
            this, &ExecutionController::onTaskLogMessage);
}

void ExecutionController::setDisplayGroupKey(const std::string& key) {
    currentDisplayGroupKey_ = key.empty() ? std::string{"default"} : key;
}

const std::string& ExecutionController::displayGroupKey() const {
    return currentDisplayGroupKey_;
}

void ExecutionController::trackTask(const QString& taskId, const QString& group, bool wasRunning) {
    if (taskId.isEmpty()) return;
    pendingResultTaskIds_.insert(taskId, group);
    if (!wasRunning && currentTaskId_.isEmpty()) {
        currentTaskId_ = taskId;
    }
}

bool ExecutionController::hasPendingExecution() const {
    const QString currentGroup = QString::fromStdString(currentDisplayGroupKey_);
    if (!currentTaskId_.isEmpty() && pendingResultTaskIds_.value(currentTaskId_) == currentGroup) {
        return true;
    }
    for (auto it = pendingResultTaskIds_.cbegin(); it != pendingResultTaskIds_.cend(); ++it) {
        if (it.value() == currentGroup) {
            return true;
        }
    }
    return false;
}

int ExecutionController::pendingCountForCurrentGroup() const {
    const QString currentGroup = QString::fromStdString(currentDisplayGroupKey_);
    int count = 0;
    for (auto it = pendingResultTaskIds_.cbegin(); it != pendingResultTaskIds_.cend(); ++it) {
        if (it.value() == currentGroup) {
            ++count;
        }
    }
    return count;
}

void ExecutionController::discardTaskState(const QStringList& taskIds) {
    bool clearedActiveTask = false;
    for (const QString& taskId : taskIds) {
        pendingResultTaskIds_.remove(taskId);
        if (taskId == currentTaskId_) {
            currentTaskId_.clear();
            clearedActiveTask = true;
        }
    }
    if (clearedActiveTask) {
        destroyProgressDialog();
        emit activeTaskDiscarded();
    }
}

QString ExecutionController::currentTaskId() const {
    return currentTaskId_;
}

bool ExecutionController::isTaskTracked(const QString& taskId) const {
    return pendingResultTaskIds_.contains(taskId);
}

QString ExecutionController::taskGroup(const QString& taskId) const {
    return pendingResultTaskIds_.value(taskId);
}

bool ExecutionController::lastSuccess() const {
    return lastExecutionSuccess_;
}

bool ExecutionController::lastCancelled() const {
    return lastExecutionCancelled_;
}

QString ExecutionController::lastMessage() const {
    return lastExecutionMessage_;
}

QString ExecutionController::lastRawMessage() const {
    return lastExecutionRawMessage_;
}

ProgressDialog* ExecutionController::progressDialog() const {
    return progressDialog_;
}

void ExecutionController::ensureProgressDialog(QWidget* parent) {
    destroyProgressDialog();
    progressDialog_ = new ProgressDialog(parent);
    progressDialog_->setAttribute(Qt::WA_DeleteOnClose);
    connect(progressDialog_, &QObject::destroyed, this, [this]() {
        progressDialog_ = nullptr;
    });
    connect(progressDialog_, &ProgressDialog::rejected, this, [this]() {
        if (!currentTaskId_.isEmpty()) {
            TaskRunner::instance().cancelTask(currentTaskId_);
        }
    });
}

void ExecutionController::destroyProgressDialog() {
    if (progressDialog_) {
        progressDialog_->deleteLater();
        progressDialog_ = nullptr;
    }
}

bool ExecutionController::affectsCurrentGroup(const QString& group) const {
    return group == QString::fromStdString(currentDisplayGroupKey_);
}

void ExecutionController::onTaskRunnerStarted(const QString& displayGroup, const QString& taskId) {
    if (currentTaskId_.isEmpty() && pendingResultTaskIds_.contains(taskId)) {
        currentTaskId_ = taskId;
        emit activeTaskStarted(taskId, affectsCurrentGroup(displayGroup));
    }
}

void ExecutionController::onTaskRunnerFinished(const QString& displayGroup,
                                                const QString& taskId,
                                                bool success,
                                                bool cancelled) {
    if (!pendingResultTaskIds_.contains(taskId)) return;

    const QString taskGrp = pendingResultTaskIds_.value(taskId);
    pendingResultTaskIds_.remove(taskId);

    const auto rec = TaskManager::instance().findTask(displayGroup, taskId);
    const QString message = rec.resultMessage.isEmpty()
        ? (success ? QStringLiteral("执行成功") : QStringLiteral("执行失败"))
        : rec.resultMessage;

    lastExecutionSuccess_ = success;
    lastExecutionCancelled_ = cancelled;
    lastExecutionMessage_ = message;
    lastExecutionRawMessage_ = rec.resultRawMessage.isEmpty() ? message : rec.resultRawMessage;

    const bool wasCurrentTask = (taskId == currentTaskId_);
    if (wasCurrentTask) {
        currentTaskId_.clear();
    }

    emit taskCompleted(taskId, affectsCurrentGroup(taskGrp),
                       success, cancelled, message, lastExecutionRawMessage_,
                       wasCurrentTask);

    if (wasCurrentTask) {
        emit executionFinished(success);
    }
}

void ExecutionController::onTaskProgressChanged(const QString& taskId, double percent) {
    const QString group = pendingResultTaskIds_.value(taskId);
    emit progressChanged(taskId, affectsCurrentGroup(group), percent);
}

void ExecutionController::onTaskLogMessage(const QString& displayGroup,
                                            const QString& taskId,
                                            const QString& msg) {
    emit logMessage(taskId, affectsCurrentGroup(displayGroup), msg);
}

}
