#include "task_runner.h"

#include "execute_worker.h"
#include "qt_progress_reporter.h"
#include "task_manager.h"
#include "gui_data_support.h"

#include <QThread>

namespace gis_ai::gui {

TaskRunner& TaskRunner::instance() {
    static TaskRunner inst;
    return inst;
}

TaskRunner::TaskRunner(QObject* parent)
    : QObject(parent) {}

QString TaskRunner::run(const QString& displayGroup,
                        const QString& pluginName,
                        const QString& actionKey,
                        const std::map<std::string, ParamValue>& params,
                        const QString& pluginDisplayName,
                        const QString& actionDisplayName) {
    TaskManager::instance().initializeGroup(displayGroup);

    const QString taskId = TaskManager::instance().submitTask(
        displayGroup, pluginName, actionKey, params, pluginDisplayName, actionDisplayName);
    if (taskId.isEmpty()) {
        return {};
    }

    QueuedTask queuedTask;
    queuedTask.displayGroup = displayGroup;
    queuedTask.pluginName = pluginName;
    queuedTask.actionKey = actionKey;
    queuedTask.params = params;
    queuedTask.pluginDisplayName = pluginDisplayName;
    queuedTask.actionDisplayName = actionDisplayName;
    queuedTask.taskId = taskId;

    queue_.enqueue(queuedTask);
    emit queueChanged(queue_.size());
    processQueue();
    return taskId;
}

void TaskRunner::processQueue() {
    if (!runningTaskId_.isEmpty() || queue_.isEmpty()) {
        return;
    }

    const QueuedTask task = queue_.dequeue();
    emit queueChanged(queue_.size());
    startTask(task);
}

void TaskRunner::startTask(const QueuedTask& task) {
    auto reporter = std::make_unique<QtProgressReporter>(task.taskId);
    auto* reporterPtr = reporter.get();

    auto ctx = std::make_shared<TaskContext>();
    ctx->taskId = task.taskId;
    ctx->displayGroup = task.displayGroup;
    ctx->reporter = std::move(reporter);
    activeTasks_[task.taskId] = ctx;
    runningTaskId_ = task.taskId;

    auto* worker = new ExecuteWorker;
    worker->setup(task.pluginName, task.actionKey, task.params, reporterPtr);

    auto* thread = new QThread;
    worker->moveToThread(thread);

    connect(reporterPtr, &QtProgressReporter::progressChanged,
            this, [this](const QString& tid, double percent) {
                emit taskProgressChanged(tid, percent);
            });

    connect(reporterPtr, &QtProgressReporter::messageLogged,
            this, [this](const QString& tid, const QString& msg) {
                auto it = activeTasks_.find(tid);
                const QString group = (it != activeTasks_.end()) ? it.value()->displayGroup : QString();
                TaskManager::instance().appendLog(group, tid, msg);
                emit taskLogMessage(group, tid, msg);
            });

    connect(thread, &QThread::started, this, [this, task]() {
        TaskManager::instance().updateTaskStatus(task.displayGroup, task.taskId, TaskRecord::Running);
        emit taskStarted(task.displayGroup, task.taskId);
    });
    connect(thread, &QThread::started, worker, &ExecuteWorker::run);

    connect(worker, &ExecuteWorker::finished, this,
            [this, task](bool success, const QString& message) {
                auto it = activeTasks_.find(task.taskId);
                const bool cancelled = (it != activeTasks_.end() && it.value()->reporter)
                    ? it.value()->reporter->isCancelled()
                    : false;

                const std::string localizedMsg = localizeResultMessage(message.toStdString());
                const QString localizedQMsg = QString::fromStdString(localizedMsg);

                TaskManager::instance().finishTask(
                    task.displayGroup, task.taskId, success, cancelled,
                    localizedQMsg, message, {});
                emit taskFinished(task.displayGroup, task.taskId, success, cancelled);

                activeTasks_.remove(task.taskId);
                if (runningTaskId_ == task.taskId) {
                    runningTaskId_.clear();
                }
                processQueue();
            });

    connect(worker, &ExecuteWorker::finished, thread, &QThread::quit);
    connect(worker, &ExecuteWorker::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void TaskRunner::cancelTask(const QString& taskId) {
    auto it = activeTasks_.find(taskId);
    if (it != activeTasks_.end() && it.value()->reporter) {
        it.value()->reporter->cancel();
    }
}

bool TaskRunner::isRunning() const {
    return !runningTaskId_.isEmpty();
}

QString TaskRunner::runningTaskId() const {
    return runningTaskId_;
}

int TaskRunner::queuedCount() const {
    return queue_.size();
}

}  // namespace gis_ai::gui
