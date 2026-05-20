#include "batch_mode_controller.h"

#include <QCoreApplication>
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace gis_ai::gui {

BatchModeController::BatchModeController(QObject* parent)
    : QObject(parent) {}

QWidget* BatchModeController::createWidget(QWidget* parent) {
    parentWidget_ = parent;
    batchControlsWidget_ = new QWidget(parent);
    auto* batchLayout = new QHBoxLayout(batchControlsWidget_);
    batchLayout->setSpacing(8);
    batchLayout->setContentsMargins(0, 0, 0, 0);

    batchCheckBox_ = new QCheckBox(QCoreApplication::translate("GuiDataSupport", "批量处理"), parent);
    batchCheckBox_->setToolTip(QCoreApplication::translate("GuiDataSupport", "启用后可对目录下匹配文件批量执行"));
    batchLayout->addWidget(batchCheckBox_);

    batchDirEdit_ = new QLineEdit(parent);
    batchDirEdit_->setPlaceholderText(QCoreApplication::translate("GuiDataSupport", "批量输入目录"));
    batchDirEdit_->setEnabled(false);
    batchLayout->addWidget(batchDirEdit_, 1);

    batchDirButton_ = new QPushButton(QStringLiteral("..."), parent);
    batchDirButton_->setObjectName(QStringLiteral("browseButton"));
    batchDirButton_->setFixedWidth(36);
    batchDirButton_->setEnabled(false);
    batchDirButton_->setToolTip(QCoreApplication::translate("GuiDataSupport", "浏览选择批量输入目录"));
    connect(batchDirButton_, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(
            parentWidget_, QCoreApplication::translate("GuiDataSupport", "选择批量输入目录"));
        if (!dir.isEmpty()) {
            batchDirEdit_->setText(dir);
        }
    });
    batchLayout->addWidget(batchDirButton_);

    batchFilterEdit_ = new QLineEdit(parent);
    batchFilterEdit_->setPlaceholderText(QStringLiteral("*.tif"));
    batchFilterEdit_->setText(QStringLiteral("*.tif"));
    batchFilterEdit_->setMaximumWidth(100);
    batchFilterEdit_->setEnabled(false);
    batchLayout->addWidget(batchFilterEdit_);

    batchCountLabel_ = new QLabel(parent);
    batchCountLabel_->setObjectName(QStringLiteral("paramDesc"));
    batchCountLabel_->setMinimumWidth(60);
    batchLayout->addWidget(batchCountLabel_);

    connect(batchCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        batchDirEdit_->setEnabled(checked);
        batchDirButton_->setEnabled(checked);
        batchFilterEdit_->setEnabled(checked);
        if (!checked) {
            batchCountLabel_->clear();
        } else {
            updateCount();
        }
        emit stateChanged();
    });

    connect(batchDirEdit_, &QLineEdit::textChanged, this, [this](const QString&) {
        updateCount();
        emit stateChanged();
    });

    connect(batchFilterEdit_, &QLineEdit::textChanged, this, [this](const QString&) {
        updateCount();
    });

    return batchControlsWidget_;
}

bool BatchModeController::setBatchMode(bool enabled) {
    if (!batchCheckBox_) return false;
    batchCheckBox_->setChecked(enabled);
    return true;
}

bool BatchModeController::setBatchInputDirectory(const std::string& directory) {
    if (!batchDirEdit_) return false;
    batchDirEdit_->setText(QString::fromStdString(directory));
    return true;
}

bool BatchModeController::setBatchFilter(const std::string& filter) {
    if (!batchFilterEdit_) return false;
    batchFilterEdit_->setText(QString::fromStdString(filter));
    return true;
}

bool BatchModeController::supportsGenericBatchMode(const std::string& pluginName,
                                                    const std::string& actionKey) const {
    const bool isBatchPlugin = pluginName == "batch";
    const bool isBatchAction = actionKey.rfind("batch_", 0) == 0;
    return !isBatchPlugin && !isBatchAction;
}

bool BatchModeController::genericBatchModeEnabled() const {
    return batchCheckBox_ && batchCheckBox_->isChecked()
        && batchControlsWidget_ && batchControlsWidget_->isVisible();
}

void BatchModeController::updateVisibility(const std::string& pluginName,
                                            const std::string& actionKey) {
    const bool visible = supportsGenericBatchMode(pluginName, actionKey);
    if (batchControlsWidget_) {
        batchControlsWidget_->setVisible(visible);
    }
    if (!visible && batchCheckBox_ && batchCheckBox_->isChecked()) {
        batchCheckBox_->setChecked(false);
    }
    if (!visible && batchCountLabel_) {
        batchCountLabel_->clear();
    }
}

void BatchModeController::updateCount() {
    if (!genericBatchModeEnabled()) {
        if (batchCountLabel_) batchCountLabel_->clear();
        return;
    }

    QString dirPath = batchDir();
    if (dirPath.isEmpty()) {
        if (batchCountLabel_)
            batchCountLabel_->setText(QCoreApplication::translate("GuiDataSupport", "0 个文件"));
        return;
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        if (batchCountLabel_)
            batchCountLabel_->setText(QCoreApplication::translate("GuiDataSupport", "目录不存在"));
        return;
    }

    QString pattern = batchFilter();
    if (pattern.isEmpty()) pattern = QStringLiteral("*.tif");

    QStringList nameFilters;
    nameFilters << pattern;
    QStringList files = dir.entryList(nameFilters, QDir::Files);
    if (batchCountLabel_)
        batchCountLabel_->setText(QCoreApplication::translate("GuiDataSupport", "%1 个文件").arg(files.size()));
}

QString BatchModeController::batchDir() const {
    return batchDirEdit_ ? batchDirEdit_->text().trimmed() : QString();
}

QString BatchModeController::batchFilter() const {
    return batchFilterEdit_ ? batchFilterEdit_->text().trimmed() : QString();
}

QStringList BatchModeController::batchFiles() const {
    QString dirPath = batchDir();
    if (dirPath.isEmpty()) return {};

    QDir dir(dirPath);
    if (!dir.exists()) return {};

    QString pattern = batchFilter();
    if (pattern.isEmpty()) pattern = QStringLiteral("*.tif");

    return dir.entryList(QStringList{pattern}, QDir::Files);
}

}
