#include "execute_worker.h"
#include "qt_progress_reporter.h"

#include <QThread>
#include <exception>

namespace gis_ai::gui {

ExecuteWorker::ExecuteWorker(QObject* parent)
    : QObject(parent) {}

void ExecuteWorker::setup(const QString& pluginName,
                           const QString& actionKey,
                           const std::map<std::string, ParamValue>& params,
                           QtProgressReporter* reporter) {
    pluginName_ = pluginName;
    actionKey_ = actionKey;
    params_ = params;
    reporter_ = reporter;
}

void ExecuteWorker::run() {
    try {
        if (reporter_ && reporter_->isCancelled()) {
            emit finished(false, QStringLiteral("任务已取消"));
            return;
        }

        emit finished(false, QStringLiteral("执行逻辑尚未实现"));
    } catch (const std::exception& e) {
        emit finished(false, QString::fromUtf8(e.what()));
    } catch (...) {
        emit finished(false, QStringLiteral("未知异常"));
    }
}

}
