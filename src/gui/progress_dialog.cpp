#include "progress_dialog.h"
#include "style_constants.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <algorithm>

namespace gis_ai::gui {

ProgressDialog::ProgressDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QCoreApplication::translate("GuiDataSupport", "执行中..."));
    setMinimumWidth(450);
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    setStyleSheet(globalStyleSheet());

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    statusLabel_ = new QLabel(QCoreApplication::translate("GuiDataSupport", "正在执行，请稍候..."));
    statusLabel_->setObjectName(QStringLiteral("progressStatusMeta"));
    layout->addWidget(statusLabel_);

    progressBar_ = new QProgressBar;
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    layout->addWidget(progressBar_);

    logEdit_ = new QTextEdit;
    logEdit_->setReadOnly(true);
    logEdit_->setMaximumHeight(150);
    logEdit_->setObjectName(QStringLiteral("logTerminal"));
    layout->addWidget(logEdit_);

    auto* btnBox = new QDialogButtonBox;
    cancelButton_ = new QPushButton(QCoreApplication::translate("GuiDataSupport", "取消"));
    cancelButton_->setObjectName(QStringLiteral("secondaryButton"));
    forceQuitButton_ = new QPushButton(QCoreApplication::translate("GuiDataSupport", "强制终止"));
    forceQuitButton_->setObjectName(QStringLiteral("secondaryButton"));
    forceQuitButton_->setVisible(false);
    btnBox->addButton(cancelButton_, QDialogButtonBox::ActionRole);
    btnBox->addButton(forceQuitButton_, QDialogButtonBox::DestructiveRole);
    layout->addWidget(btnBox);

    connect(cancelButton_, &QPushButton::clicked, this, [this]() {
        cancelButton_->setEnabled(false);
        statusLabel_->setText(QCoreApplication::translate("GuiDataSupport", "正在取消..."));
    });

    connect(forceQuitButton_, &QPushButton::clicked, this, &QDialog::reject);
}

void ProgressDialog::setFinished(const QString& message, bool success, bool cancelled) {
    if (cancelled) {
        statusLabel_->setText(QCoreApplication::translate("GuiDataSupport", "已取消"));
    } else if (success) {
        statusLabel_->setText(QCoreApplication::translate("GuiDataSupport", "执行完成"));
    } else {
        statusLabel_->setText(QCoreApplication::translate("GuiDataSupport", "执行失败"));
    }

    progressBar_->setRange(0, 100);
    progressBar_->setValue(success ? 100 : 0);

    cancelButton_->setVisible(false);
    forceQuitButton_->setVisible(false);

    auto* closeBtn = new QPushButton(QCoreApplication::translate("GuiDataSupport", "关闭"));
    closeBtn->setObjectName(QStringLiteral("primaryButton"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    auto* btnBox = findChild<QDialogButtonBox*>();
    if (btnBox) {
        btnBox->clear();
        btnBox->addButton(closeBtn, QDialogButtonBox::AcceptRole);
    }
}

void ProgressDialog::updateProgress(double percent) {
    int value = std::clamp(static_cast<int>(percent * 100.0), 0, 100);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(value);
}

void ProgressDialog::appendLog(const QString& message) {
    logEdit_->append(message.toHtmlEscaped());
}

}
