#pragma once

#include <QObject>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>

namespace gis_ai::gui {

class QtProgressReporter : public QObject {
    Q_OBJECT
public:
    explicit QtProgressReporter(const QString& taskId, QObject* parent = nullptr);

    virtual void onProgress(double percent);
    virtual void onMessage(const std::string& msg);
    bool isCancelled() const;

    void cancel();
    void reset();

    QString taskId() const { return taskId_; }

signals:
    void progressChanged(const QString& taskId, double percent);
    void messageLogged(const QString& taskId, const QString& msg);

private:
    bool shouldEmitProgress(double percent) const;

    std::atomic<bool> m_cancelled{false};
    double m_lastProgressValue{-1.0};
    std::chrono::steady_clock::time_point m_lastProgressTime{};
    bool m_firstProgress{true};
    QString taskId_;

    static constexpr double kProgressDelta = 0.01;
    static constexpr int kProgressIntervalMs = 50;
};

}
