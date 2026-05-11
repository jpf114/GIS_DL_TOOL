#include "execute_worker.h"
#include "qt_progress_reporter.h"
#include "gui_data_support.h"

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

        if (!reporter_) {
            emit finished(false, QStringLiteral("进度报告器未初始化"));
            return;
        }

        bool success = executeAction(
            pluginName_.toStdString(),
            actionKey_.toStdString(),
            params_,
            *reporter_);

        if (reporter_->isCancelled()) {
            emit finished(false, QStringLiteral("任务已取消"));
            return;
        }

        if (success) {
            emit finished(true, QStringLiteral("执行成功"));
        } else {
            emit finished(false, QStringLiteral("执行失败"));
        }
    } catch (const std::exception& e) {
        emit finished(false, QString::fromUtf8(e.what()));
    } catch (...) {
        emit finished(false, QStringLiteral("未知异常"));
    }
}

}
