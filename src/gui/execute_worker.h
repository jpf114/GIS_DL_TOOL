#pragma once

#include <QObject>
#include <QString>
#include <map>
#include <string>
#include "param_card_widget.h"

namespace gis_ai::gui {

class QtProgressReporter;

class ExecuteWorker : public QObject {
    Q_OBJECT
public:
    explicit ExecuteWorker(QObject* parent = nullptr);

    void setup(const QString& pluginName,
               const QString& actionKey,
               const std::map<std::string, ParamValue>& params,
               QtProgressReporter* reporter);

public slots:
    void run();

signals:
    void finished(bool success, const QString& message);

private:
    QString pluginName_;
    QString actionKey_;
    std::map<std::string, ParamValue> params_;
    QtProgressReporter* reporter_ = nullptr;
};

}
