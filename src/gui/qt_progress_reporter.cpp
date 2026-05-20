#include "qt_progress_reporter.h"

namespace gis_ai::gui {

QtProgressReporter::QtProgressReporter(const QString& taskId, QObject* parent)
    : QObject(parent), taskId_(taskId) {}

bool QtProgressReporter::shouldEmitProgress(double percent) const {
    std::lock_guard<std::mutex> lock(m_progressMutex);
    if (m_firstProgress) return true;
    if (percent >= 1.0) return true;
    if (percent <= 0.0) return true;

    double delta = std::abs(percent - m_lastProgressValue);
    if (delta >= kProgressDelta) return true;

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - m_lastProgressTime).count();
    if (elapsed >= kProgressIntervalMs) return true;

    return false;
}

void QtProgressReporter::onProgress(double percent) {
    bool should_emit = false;
    {
        std::lock_guard<std::mutex> lock(m_progressMutex);
        if (m_firstProgress || percent >= 1.0 || percent <= 0.0) {
            should_emit = true;
        } else {
            double delta = std::abs(percent - m_lastProgressValue);
            if (delta >= kProgressDelta) {
                should_emit = true;
            } else {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - m_lastProgressTime).count();
                if (elapsed >= kProgressIntervalMs) {
                    should_emit = true;
                }
            }
        }

        if (should_emit) {
            m_lastProgressValue = percent;
            m_lastProgressTime = std::chrono::steady_clock::now();
            m_firstProgress = false;
        }
    }

    if (should_emit) {
        emit progressChanged(taskId_, percent);
    }
}

void QtProgressReporter::onMessage(const std::string& msg) {
    emit messageLogged(taskId_, QString::fromUtf8(msg.c_str()));
}

bool QtProgressReporter::isCancelled() const {
    return m_cancelled.load(std::memory_order_relaxed);
}

void QtProgressReporter::cancel() {
    m_cancelled.store(true, std::memory_order_relaxed);
}

void QtProgressReporter::reset() {
    m_cancelled.store(false, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(m_progressMutex);
        m_lastProgressValue = -1.0;
        m_firstProgress = true;
        m_lastProgressTime = std::chrono::steady_clock::now();
    }
}

}
