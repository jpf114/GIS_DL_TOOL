#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <string>

class QCheckBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QWidget;

namespace gis_ai::gui {

class BatchModeController : public QObject {
    Q_OBJECT
public:
    explicit BatchModeController(QObject* parent = nullptr);

    QWidget* createWidget(QWidget* parent);

    bool setBatchMode(bool enabled);
    bool setBatchInputDirectory(const std::string& directory);
    bool setBatchFilter(const std::string& filter);

    bool supportsGenericBatchMode(const std::string& pluginName,
                                   const std::string& actionKey) const;
    bool genericBatchModeEnabled() const;

    void updateVisibility(const std::string& pluginName, const std::string& actionKey);
    void updateCount();

    QString batchDir() const;
    QString batchFilter() const;
    QStringList batchFiles() const;

signals:
    void stateChanged();

private:
    QWidget* parentWidget_ = nullptr;
    QWidget* batchControlsWidget_ = nullptr;
    QCheckBox* batchCheckBox_ = nullptr;
    QLineEdit* batchDirEdit_ = nullptr;
    QPushButton* batchDirButton_ = nullptr;
    QLineEdit* batchFilterEdit_ = nullptr;
    QLabel* batchCountLabel_ = nullptr;
};

}
