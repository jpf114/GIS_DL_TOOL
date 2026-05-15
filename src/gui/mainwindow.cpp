#include "mainwindow.h"
#include "style_constants.h"
#include "nav_panel.h"
#include "icon_manager.h"
#include "param_widget.h"
#include "param_card_widget.h"
#include "task_center_page.h"
#include "task_manager.h"
#include "task_runner.h"
#include "settings_manager.h"
#include "gdal_config.h"
#include "gui_data_support.h"
#include "progress_dialog.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>
#include <algorithm>

namespace gis_ai::gui {

namespace {

QPixmap defaultBadgePixmap() {
    const int size = 38;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#EAF3FF"));
    painter.drawRoundedRect(QRectF(0.5, 0.5, size - 1.0, size - 1.0), 8, 8);
    QPen pen(QColor("#2F7CF6"));
    pen.setWidthF(1.8);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.drawEllipse(QRectF(11, 11, 16, 16));
    return pixmap;
}

QIcon executeIcon() {
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    QPolygonF triangle;
    triangle << QPointF(4.0, 2.5) << QPointF(13.0, 8.0) << QPointF(4.0, 13.5);
    painter.drawPolygon(triangle);
    return QIcon(pixmap);
}

std::string pluginDisplayName(const std::string& key) {
    if (key == "segment")    return "大图分割";
    if (key == "inference")  return "模型推理";
    if (key == "preprocess") return "数据预处理";
    if (key == "vector")     return "矢量处理";
    if (key == "raster")     return "栅格处理";
    if (key == "batch")      return "批量处理";
    return key;
}

QString deriveOutputPath(const QString& inputPath, const QString& suffix) {
    if (inputPath.isEmpty()) return {};
    QFileInfo fi(inputPath);
    QString baseDir = fi.absolutePath();
    QString baseName = fi.completeBaseName();
    return baseDir + QStringLiteral("/") + baseName + suffix;
}

QString suffixForAction(const std::string& pluginName, const std::string& actionKey) {
    if (pluginName == "segment") return QStringLiteral("_segment");
    if (pluginName == "inference") return QStringLiteral("_inference");
    if (pluginName == "preprocess") return QStringLiteral("_preprocess");
    if (pluginName == "vector") return QStringLiteral("_vector");
    if (pluginName == "raster") return QStringLiteral("_raster");
    if (pluginName == "batch") return QStringLiteral("_batch");
    return QStringLiteral("_result");
}

}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setAcceptDrops(true);
    setupUi();
}

MainWindow::~MainWindow() = default;

void MainWindow::selectPluginByName(const std::string& pluginName) {
    if (navPanel_) {
        navPanel_->setCurrentPluginSelection(pluginName);
    }
    onPluginSelected(pluginName);
}

void MainWindow::selectActionByKey(const std::string& actionKey) {
    if (currentPluginName_.empty()) {
        return;
    }
    if (navPanel_) {
        navPanel_->setCurrentSubFunctionSelection(currentPluginName_, actionKey);
    }
    onSubFunctionSelected(currentPluginName_, actionKey);
}

bool MainWindow::setParamValue(const std::string& key, const std::string& value) {
    if (!paramWidget_ || !paramWidget_->hasParam(key)) {
        return false;
    }
    paramWidget_->setValueFromString(key, value);
    return true;
}

bool MainWindow::setBatchMode(bool enabled) {
    if (!batchCheckBox_) {
        return false;
    }
    batchCheckBox_->setChecked(enabled);
    return true;
}

bool MainWindow::setBatchInputDirectory(const std::string& directory) {
    if (!batchDirEdit_) {
        return false;
    }
    batchDirEdit_->setText(QString::fromStdString(directory));
    return true;
}

bool MainWindow::setBatchFilter(const std::string& filter) {
    if (!batchFilterEdit_) {
        return false;
    }
    batchFilterEdit_->setText(QString::fromStdString(filter));
    return true;
}

void MainWindow::triggerExecute() {
    onExecuteClicked();
}

bool MainWindow::lastExecutionSuccess() const {
    return lastExecutionSuccess_;
}

bool MainWindow::lastExecutionCancelled() const {
    return lastExecutionCancelled_;
}

QString MainWindow::lastExecutionMessage() const {
    return lastExecutionMessage_;
}

QString MainWindow::lastExecutionRawMessage() const {
    return lastExecutionRawMessage_;
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("GIS AI 工具台"));
    resize(Size::kWindowDefaultWidth, Size::kWindowDefaultHeight);
    setMinimumSize(Size::kWindowMinWidth, Size::kWindowMinHeight);
    setStyleSheet(globalStyleSheet());

    QDir appDir(QApplication::applicationDirPath());
    QString iconsBasePath;
    QDir searchDir = appDir;
    for (int i = 0; i < 6; ++i) {
        QString candidate = searchDir.absoluteFilePath(QStringLiteral("share/icons"));
        if (QDir(candidate).exists()) {
            iconsBasePath = candidate;
            break;
        }
        if (!searchDir.cdUp()) break;
    }
    if (iconsBasePath.isEmpty()) {
        QDir devDir = appDir;
        if (devDir.cdUp()) {
            QString candidate = devDir.absoluteFilePath(QStringLiteral("resources/icons"));
            if (QDir(candidate).exists()) {
                iconsBasePath = candidate;
            }
        }
    }
    if (iconsBasePath.isEmpty()) {
        iconsBasePath = appDir.absoluteFilePath(QStringLiteral("../resources/icons"));
    }
    IconManager::instance().setIconsBasePath(iconsBasePath.toStdString());

    TaskManager::instance().initializeGroup(QStringLiteral("default"));

    auto* centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    navPanel_ = new NavPanel;
    connect(navPanel_, &NavPanel::pluginSelected,
            this, &MainWindow::onPluginSelected);
    connect(navPanel_, &NavPanel::subFunctionSelected,
            this, &MainWindow::onSubFunctionSelected);

    auto* rightPanel = new QWidget;
    rightPanel->setObjectName(QStringLiteral("pagePanel"));

    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 18);
    rightLayout->setSpacing(Size::kCardSpacing);

    auto* titleCard = new QFrame;
    titleCard->setObjectName(QStringLiteral("heroCard"));
    auto* titleLayout = new QVBoxLayout(titleCard);
    titleLayout->setContentsMargins(22, 20, 22, 20);
    titleLayout->setSpacing(10);

    auto* headerTopLayout = new QHBoxLayout;
    headerTopLayout->setContentsMargins(0, 0, 0, 0);
    headerTopLayout->setSpacing(10);

    auto* heroBadgeLabel = new QLabel(QStringLiteral("AI 工作台"));
    heroBadgeLabel->setObjectName(QStringLiteral("heroBadge"));
    headerTopLayout->addWidget(heroBadgeLabel, 0, Qt::AlignLeft);
    headerTopLayout->addStretch();
    titleLayout->addLayout(headerTopLayout);

    auto* heroMainLayout = new QHBoxLayout;
    heroMainLayout->setContentsMargins(0, 0, 0, 0);
    heroMainLayout->setSpacing(12);

    functionIconLabel_ = new QLabel;
    functionIconLabel_->setObjectName(QStringLiteral("heroIconBadge"));
    functionIconLabel_->setAlignment(Qt::AlignCenter);
    functionIconLabel_->setPixmap(defaultBadgePixmap());
    heroMainLayout->addWidget(functionIconLabel_, 0, Qt::AlignTop);

    auto* heroTextLayout = new QVBoxLayout;
    heroTextLayout->setContentsMargins(0, 0, 0, 0);
    heroTextLayout->setSpacing(2);

    functionTitleLabel_ = new QLabel(QStringLiteral("请选择功能"));
    functionTitleLabel_->setObjectName(QStringLiteral("heroTitle"));
    heroTextLayout->addWidget(functionTitleLabel_);

    functionDescLabel_ = new QLabel(
        QStringLiteral("从左侧选择主功能和子功能后，这里会显示功能说明和参数配置。"));
    functionDescLabel_->setObjectName(QStringLiteral("heroDesc"));
    functionDescLabel_->setWordWrap(true);
    heroTextLayout->addWidget(functionDescLabel_);

    functionMetaLabel_ = new QLabel(QStringLiteral("当前状态：等待选择主功能"));
    functionMetaLabel_->setObjectName(QStringLiteral("mainHeroMeta"));
    heroTextLayout->addWidget(functionMetaLabel_);

    heroMainLayout->addLayout(heroTextLayout, 1);
    titleLayout->addLayout(heroMainLayout);

    rightLayout->addWidget(titleCard);

    auto* paramScrollArea = new QScrollArea;
    paramScrollArea->setWidgetResizable(true);
    paramScrollArea->setFrameShape(QFrame::NoFrame);
    paramScrollArea->setStyleSheet(
        QStringLiteral("QScrollArea { background: transparent; border: none; }"));

    paramWidget_ = new ParamWidget;
    paramScrollArea->setWidget(paramWidget_);

    connect(paramWidget_, &ParamWidget::paramsChanged,
            this, &MainWindow::onParamsChanged);

    rightLayout->addWidget(paramScrollArea, 1);

    auto* executionCard = new QFrame;
    executionCard->setObjectName(QStringLiteral("execCard"));
    auto* executionLayout = new QVBoxLayout(executionCard);
    executionLayout->setContentsMargins(18, 18, 18, 18);
    executionLayout->setSpacing(12);

    auto* execHeaderLayout = new QHBoxLayout;
    execHeaderLayout->setSpacing(12);

    auto* execTitleLabel = new QLabel(QStringLiteral("执行控制"));
    execTitleLabel->setObjectName(QStringLiteral("cardTitle"));
    execHeaderLayout->addWidget(execTitleLabel);
    execHeaderLayout->addStretch();

    executeButton_ = new QPushButton(QStringLiteral("执行处理"));
    executeButton_->setObjectName(QStringLiteral("primaryButton"));
    executeButton_->setIcon(executeIcon());
    executeButton_->setIconSize(QSize(16, 16));
    executeButton_->setEnabled(false);
    executeButton_->setToolTip(QStringLiteral("请先选择主功能和子功能"));
    connect(executeButton_, &QPushButton::clicked, this, &MainWindow::onExecuteClicked);
    execHeaderLayout->addWidget(executeButton_);

    executionLayout->addLayout(execHeaderLayout);

    auto* batchLayout = new QHBoxLayout;
    batchLayout->setSpacing(8);

    batchCheckBox_ = new QCheckBox(QStringLiteral("批量处理"));
    batchCheckBox_->setToolTip(QStringLiteral("启用后可对目录下匹配文件批量执行"));
    batchLayout->addWidget(batchCheckBox_);

    batchDirEdit_ = new QLineEdit;
    batchDirEdit_->setPlaceholderText(QStringLiteral("批量输入目录"));
    batchDirEdit_->setEnabled(false);
    batchLayout->addWidget(batchDirEdit_, 1);

    batchDirButton_ = new QPushButton(QStringLiteral("..."));
    batchDirButton_->setObjectName(QStringLiteral("browseButton"));
    batchDirButton_->setFixedWidth(36);
    batchDirButton_->setEnabled(false);
    batchDirButton_->setToolTip(QStringLiteral("浏览选择批量输入目录"));
    connect(batchDirButton_, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择批量输入目录"));
        if (!dir.isEmpty()) {
            batchDirEdit_->setText(dir);
        }
    });
    batchLayout->addWidget(batchDirButton_);

    batchFilterEdit_ = new QLineEdit;
    batchFilterEdit_->setPlaceholderText(QStringLiteral("*.tif"));
    batchFilterEdit_->setText(QStringLiteral("*.tif"));
    batchFilterEdit_->setMaximumWidth(100);
    batchFilterEdit_->setEnabled(false);
    batchLayout->addWidget(batchFilterEdit_);

    batchCountLabel_ = new QLabel;
    batchCountLabel_->setObjectName(QStringLiteral("paramDesc"));
    batchCountLabel_->setMinimumWidth(60);
    batchLayout->addWidget(batchCountLabel_);

    executionLayout->addLayout(batchLayout);

    connect(batchCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        batchDirEdit_->setEnabled(checked);
        batchDirButton_->setEnabled(checked);
        batchFilterEdit_->setEnabled(checked);
        if (!checked) {
            batchCountLabel_->clear();
        } else {
            updateBatchCount();
        }
        updateExecuteButtonState();
    });

    connect(batchDirEdit_, &QLineEdit::textChanged, this, [this](const QString&) {
        updateBatchCount();
        updateExecuteButtonState();
    });

    connect(batchFilterEdit_, &QLineEdit::textChanged, this, [this](const QString&) {
        updateBatchCount();
    });

    resultSummaryLabel_ = new QLabel;
    resultSummaryLabel_->setWordWrap(true);
    resultSummaryLabel_->setObjectName(QStringLiteral("execSummary"));
    resultSummaryLabel_->setMinimumHeight(28);
    resetExecutionSummary();
    executionLayout->addWidget(resultSummaryLabel_);

    rightLayout->addWidget(executionCard);

    tabWidget_ = new QTabWidget;
    tabWidget_->setObjectName(QStringLiteral("pagePanel"));
    tabWidget_->setTabPosition(QTabWidget::North);
    tabWidget_->addTab(rightPanel, QStringLiteral("功能配置"));

    taskCenterPage_ = new TaskCenterPage;
    taskCenterPage_->setCurrentGroup(QString::fromStdString(currentDisplayGroupKey_));
    tabWidget_->addTab(taskCenterPage_, QStringLiteral("任务中心"));

    connect(taskCenterPage_, &TaskCenterPage::clearHistoryRequested, this, [this]() {
        const QString displayGroup = taskCenterPage_->currentGroup();
        QStringList taskIds;
        const auto tasks = TaskManager::instance().recentTasks(displayGroup, 1000);
        taskIds.reserve(tasks.size());
        for (const auto& task : tasks) {
            taskIds.push_back(task.id);
        }
        discardTaskExecutionState(taskIds);
        TaskManager::instance().clearHistory(taskCenterPage_->currentGroup());
        taskCenterPage_->refreshAll();
        resetExecutionSummary();
    });

    connect(taskCenterPage_, &TaskCenterPage::deleteTasksRequested, this, [this](const QStringList& ids) {
        const QString displayGroup = taskCenterPage_->currentGroup();
        discardTaskExecutionState(ids);
        TaskManager::instance().deleteTasks(displayGroup, ids);
        taskCenterPage_->removeTaskRows(ids);
        if (TaskManager::instance().taskCount(displayGroup) == 0) {
            resetExecutionSummary();
        }
    });

    connect(taskCenterPage_, &TaskCenterPage::clearLogsRequested, this, [this](const QString& taskId) {
        TaskManager::instance().clearLogsForTask(taskCenterPage_->currentGroup(), taskId);
        taskCenterPage_->clearLogDisplay();
    });

    connect(taskCenterPage_, &TaskCenterPage::clearAllLogsRequested, this, [this]() {
        TaskManager::instance().clearAllLogs(taskCenterPage_->currentGroup());
        taskCenterPage_->clearLogDisplay();
    });

    connect(taskCenterPage_, &TaskCenterPage::rerunTaskRequested, this, &MainWindow::onRerunTask);
    connect(taskCenterPage_, &TaskCenterPage::editTaskRequested, this, &MainWindow::onEditTask);

    connect(&TaskManager::instance(), &TaskManager::taskSubmitted, this,
            [this](const QString& group, const QString& id) {
        if (group != taskCenterPage_->currentGroup()) return;
        auto rec = TaskManager::instance().findTask(group, id);
        taskCenterPage_->addTaskRow(id, rec.actionDisplayName,
            static_cast<int>(rec.status),
            rec.startTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    });

    connect(&TaskManager::instance(), &TaskManager::taskFinished, this,
            [this](const QString& group, const QString& id) {
        if (group != taskCenterPage_->currentGroup()) return;
        auto rec = TaskManager::instance().findTask(group, id);
        taskCenterPage_->updateTaskRow(id, static_cast<int>(rec.status),
            rec.endTime.toString(Qt::ISODate), rec.durationMs);
    });

    connect(&TaskRunner::instance(), &TaskRunner::taskFinished,
            this, &MainWindow::onTaskRunnerFinished);
    connect(&TaskRunner::instance(), &TaskRunner::taskStarted,
            this, &MainWindow::onTaskRunnerStarted);
    connect(&TaskRunner::instance(), &TaskRunner::taskProgressChanged,
            this, &MainWindow::onTaskProgressChanged);
    connect(&TaskRunner::instance(), &TaskRunner::taskLogMessage,
            this, &MainWindow::onTaskLogMessage);

    mainLayout->addWidget(navPanel_);
    mainLayout->addWidget(tabWidget_, 1);

    statusAlgorithmLabel_ = new QLabel(QStringLiteral("当前算法：未选择"));
    statusProgressBar_ = new QProgressBar;
    statusProgressBar_->setRange(0, 100);
    statusProgressBar_->setValue(0);
    statusProgressBar_->setFixedWidth(180);
    statusProgressBar_->setTextVisible(false);
    statusAlgorithmLabel_->setObjectName(QStringLiteral("statusBarLabel"));
    statusBar()->addPermanentWidget(statusAlgorithmLabel_);
    statusBar()->addPermanentWidget(statusProgressBar_);
    statusBar()->showMessage(QStringLiteral("就绪"));
}

void MainWindow::onPluginSelected(const std::string& pluginName) {
    currentPluginName_.clear();
    currentActionKey_.clear();
    lastAutoOutputPath_.clear();
    currentDisplayGroupKey_ = pluginName.empty() ? std::string{"default"} : pluginName;

    if (pluginName.empty()) {
        applyFunctionPanelState(
            QStringLiteral("请选择功能"),
            QStringLiteral("从左侧选择主功能和子功能后，这里会显示功能说明和参数配置。"),
            QStringLiteral("当前状态：等待选择主功能"),
            QStringLiteral("当前算法：未选择"),
            {});
        executeButton_->setEnabled(false);
        executeButton_->setToolTip(QStringLiteral("请先选择主功能和子功能"));
        if (paramWidget_) {
            paramWidget_->clear();
        }
        if (taskCenterPage_) {
            taskCenterPage_->setCurrentGroup(QString::fromStdString(currentDisplayGroupKey_));
        }
        syncExecutionSummaryForCurrentGroup();
        statusBar()->showMessage(QStringLiteral("就绪"));
        return;
    }

    currentPluginName_ = pluginName;
    const QString displayName = QString::fromStdString(pluginDisplayName(pluginName));
    applyFunctionPanelState(
        displayName,
        QStringLiteral("已选择 %1，请选择子功能进行操作。").arg(displayName),
        QStringLiteral("当前状态：已选择主功能"),
        QStringLiteral("当前算法：%1").arg(displayName),
        pluginName);
    executeButton_->setEnabled(false);
    executeButton_->setToolTip(QStringLiteral("请先选择子功能"));

    if (paramWidget_) {
        paramWidget_->clear();
    }
    if (taskCenterPage_) {
        taskCenterPage_->setCurrentGroup(QString::fromStdString(currentDisplayGroupKey_));
    }
    syncExecutionSummaryForCurrentGroup();
    if (!currentGroupHasPendingExecution() && resultSummaryLabel_) {
        resultSummaryLabel_->setStyleSheet(QString());
        resultSummaryLabel_->setText(QStringLiteral("请选择该主功能下的子功能，然后补全参数。"));
    }
    statusBar()->showMessage(QStringLiteral("当前主功能：%1").arg(displayName));
}

void MainWindow::onSubFunctionSelected(const std::string& pluginName,
                                       const std::string& actionKey) {
    currentPluginName_ = pluginName;
    currentActionKey_ = actionKey;
    lastAutoOutputPath_.clear();
    currentDisplayGroupKey_ = pluginName.empty() ? std::string{"default"} : pluginName;

    auto uiConfig = getActionUiConfig(pluginName, actionKey);
    const QString pluginDisplay = QString::fromStdString(pluginDisplayName(pluginName));
    const QString actionDisplay = uiConfig.displayName.isEmpty()
        ? QString::fromStdString(actionKey)
        : uiConfig.displayName;

    applyFunctionPanelState(
        QStringLiteral("%1 / %2").arg(pluginDisplay, actionDisplay),
        uiConfig.description,
        QStringLiteral("当前状态：已选择子功能，可配置参数"),
        QStringLiteral("当前算法：%1 / %2").arg(pluginDisplay, actionDisplay),
        pluginName);

    auto specs = buildEffectiveParamSpecs(pluginName, actionKey);
    paramWidget_->setParamSpecs(specs);
    paramWidget_->setUiContext(pluginName, actionKey);
    if (taskCenterPage_) {
        taskCenterPage_->setCurrentGroup(QString::fromStdString(currentDisplayGroupKey_));
    }
    syncExecutionSummaryForCurrentGroup();
    if (!currentGroupHasPendingExecution() && resultSummaryLabel_) {
        resultSummaryLabel_->setStyleSheet(QString());
        resultSummaryLabel_->setText(QStringLiteral("参数面板已刷新，补全必填项后即可执行当前子功能。"));
    }
    statusBar()->showMessage(QStringLiteral("当前子功能：%1").arg(actionDisplay));

    updateExecuteButtonState();
}

void MainWindow::onParamsChanged() {
    syncDerivedParams();
    updateExecuteButtonState();
}

void MainWindow::syncDerivedParams() {
    if (currentPluginName_.empty() || currentActionKey_.empty()) return;

    QString primaryInputPath;
    std::string primaryInputKey;
    if (paramWidget_->hasParam("input_raster")) {
        primaryInputKey = "input_raster";
        primaryInputPath = QString::fromStdString(paramWidget_->stringValue("input_raster"));
    } else if (paramWidget_->hasParam("input_vector")) {
        primaryInputKey = "input_vector";
        primaryInputPath = QString::fromStdString(paramWidget_->stringValue("input_vector"));
    }

    if (!primaryInputPath.isEmpty()) {
        DataAutoFillInfo autoFillInfo = inspectDataForAutoFill(primaryInputPath.toStdString());

        if (autoFillInfo.hasExtent && paramWidget_->hasParam("clip_extent")) {
            auto currentExtentStr = paramWidget_->stringValue("clip_extent");
            auto lastIt = lastAutoExtent_.find("clip_extent");
            std::optional<std::array<double, 4>> currentExtent;
            std::optional<std::array<double, 4>> lastAutoExtent;

            if (!currentExtentStr.empty()) {
                QStringList parts = QString::fromStdString(currentExtentStr).split(',', Qt::SkipEmptyParts);
                if (parts.size() == 4) {
                    std::array<double, 4> arr = {parts[0].trimmed().toDouble(),
                                                  parts[1].trimmed().toDouble(),
                                                  parts[2].trimmed().toDouble(),
                                                  parts[3].trimmed().toDouble()};
                    currentExtent = arr;
                }
            }
            if (lastIt != lastAutoExtent_.end()) lastAutoExtent = lastIt->second;

            if (shouldAutoFillExtentValue(currentExtent, lastAutoExtent, true)) {
                paramWidget_->setExtentValue("clip_extent", autoFillInfo.extent);
                lastAutoExtent_["clip_extent"] = autoFillInfo.extent;
            }
        }
    }

    auto trySetOutput = [&](const std::string& outputKey) {
        if (!paramWidget_->hasParam(outputKey)) return;

        auto currentOutput = paramWidget_->stringValue(outputKey);
        auto autoIt = lastAutoOutputPath_.find(outputKey);

        std::string lastAuto = (autoIt != lastAutoOutputPath_.end())
            ? autoIt->second.toStdString() : std::string{};

        DerivedOutputUpdate update = computeDerivedOutputUpdate(
            currentOutput, lastAuto,
            primaryInputPath.toStdString(),
            currentPluginName_, currentActionKey_, outputKey);

        if (update.shouldApply) {
            paramWidget_->setStringValue(outputKey, update.value);
            lastAutoOutputPath_[outputKey] = QString::fromStdString(update.autoValue);
        }
    };

    trySetOutput("output_tif");
    trySetOutput("output_path");
    trySetOutput("output_shp");
}

QString MainWindow::buildSuggestedOutputPath(const QString& inputPath,
                                              const QString& ext) const {
    if (inputPath.isEmpty()) return {};

    QFileInfo fi(inputPath);
    QString baseDir = fi.absolutePath();
    QString baseName = fi.completeBaseName();

    QString funcSuffix = suffixForAction(currentPluginName_, currentActionKey_);

    QString actionPart;
    if (!currentActionKey_.empty()) {
        std::string ak = currentActionKey_;
        auto sep = ak.find('_');
        if (sep != std::string::npos) {
            actionPart = QStringLiteral("_") + QString::fromStdString(ak.substr(sep + 1));
        }
    }

    return baseDir + QStringLiteral("/") + baseName + funcSuffix + actionPart + ext;
}

void MainWindow::ensureProgressDialog() {
    if (progressDialog_) {
        progressDialog_->deleteLater();
        progressDialog_ = nullptr;
    }

    progressDialog_ = new ProgressDialog(this);
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

void MainWindow::resetExecutionSummary() {
    if (resultSummaryLabel_) {
        resultSummaryLabel_->setText(
            QStringLiteral("当前未执行任务。选择子功能并补全参数后，可以直接开始运行。"));
    }
    if (statusProgressBar_) {
        statusProgressBar_->setValue(0);
    }
}

bool MainWindow::currentGroupHasPendingExecution() const {
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

void MainWindow::discardTaskExecutionState(const QStringList& taskIds) {
    bool clearedActiveTask = false;
    for (const QString& taskId : taskIds) {
        pendingResultTaskIds_.remove(taskId);
        if (taskId == currentTaskId_) {
            currentTaskId_.clear();
            clearedActiveTask = true;
        }
    }

    if (!clearedActiveTask) {
        return;
    }

    if (progressDialog_) {
        progressDialog_->deleteLater();
        progressDialog_ = nullptr;
    }
    resetExecutionSummary();
    statusBar()->showMessage(QStringLiteral("就绪"));
}

void MainWindow::showRunningTaskSummary(const QString& taskId) {
    Q_UNUSED(taskId);
    if (resultSummaryLabel_) {
        resultSummaryLabel_->setStyleSheet(QString());
        resultSummaryLabel_->setText(QStringLiteral("正在执行，请稍候..."));
    }
    if (statusProgressBar_) {
        statusProgressBar_->setValue(0);
    }
    statusBar()->showMessage(QStringLiteral("执行中"));
}

void MainWindow::showQueuedTaskSummary(const QString& taskId) {
    Q_UNUSED(taskId);
    if (resultSummaryLabel_) {
        resultSummaryLabel_->setStyleSheet(QString());
        resultSummaryLabel_->setText(QStringLiteral("已加入队列，等待当前任务完成后自动执行。"));
    }
    statusBar()->showMessage(QStringLiteral("任务已加入队列"));
}

void MainWindow::showQueuedBatchSummary(int submittedCount) {
    if (resultSummaryLabel_) {
        resultSummaryLabel_->setStyleSheet(QString());
        resultSummaryLabel_->setText(
            QStringLiteral("批量处理：已提交 %1 个任务到队列").arg(submittedCount));
    }
    if (statusProgressBar_) {
        statusProgressBar_->setValue(0);
    }
    statusBar()->showMessage(QStringLiteral("批量任务已加入队列"));
}

void MainWindow::showFinishedTaskSummary(const QString& taskId,
                                         const QString& message,
                                         bool success,
                                         bool cancelled) {
    Q_UNUSED(taskId);
    if (resultSummaryLabel_) {
        if (cancelled) {
            resultSummaryLabel_->setText(QStringLiteral("✖ 已取消\n%1").arg(message));
            resultSummaryLabel_->setStyleSheet(
                QStringLiteral("QLabel#execSummary { color: #8A5A00; }"));
        } else if (success) {
            resultSummaryLabel_->setText(QStringLiteral("✓ 执行成功\n%1").arg(message));
            resultSummaryLabel_->setStyleSheet(
                QStringLiteral("QLabel#execSummary { color: #1F7A45; }"));
        } else {
            resultSummaryLabel_->setText(QStringLiteral("✖ 执行失败\n%1").arg(message));
            resultSummaryLabel_->setStyleSheet(
                QStringLiteral("QLabel#execSummary { color: #B42318; }"));
        }
    }

    if (statusProgressBar_) {
        statusProgressBar_->setValue(success ? 100 : 0);
    }

    if (cancelled) {
        statusBar()->showMessage(QStringLiteral("执行已取消"));
    } else if (success) {
        statusBar()->showMessage(QStringLiteral("执行成功"));
    } else {
        statusBar()->showMessage(QStringLiteral("执行失败"));
    }
}

void MainWindow::applyFunctionPanelState(const QString& title,
                                         const QString& description,
                                         const QString& metaText,
                                         const QString& algorithmText,
                                         const std::string& pluginName) {
    functionTitleLabel_->setText(title);
    functionDescLabel_->setText(description);
    functionMetaLabel_->setText(metaText);
    statusAlgorithmLabel_->setText(algorithmText);

    auto& mgr = IconManager::instance();
    if (!pluginName.empty() && mgr.hasPluginIcon(pluginName)) {
        functionIconLabel_->setPixmap(mgr.pixmapForPlugin(pluginName, 38, QColor("#2F7CF6")));
    } else {
        functionIconLabel_->setPixmap(defaultBadgePixmap());
    }
}

void MainWindow::syncExecutionSummaryForCurrentGroup() {
    const QString currentGroup = QString::fromStdString(currentDisplayGroupKey_);
    if (!currentTaskId_.isEmpty() && pendingResultTaskIds_.value(currentTaskId_) == currentGroup) {
        showRunningTaskSummary(currentTaskId_);
        return;
    }

    int queuedCount = 0;
    for (auto it = pendingResultTaskIds_.cbegin(); it != pendingResultTaskIds_.cend(); ++it) {
        if (it.value() == currentGroup) {
            ++queuedCount;
        }
    }

    if (queuedCount > 0) {
        resultSummaryLabel_->setText(
            QStringLiteral("当前分组有 %1 个任务正在队列中等待执行。").arg(queuedCount));
        if (statusProgressBar_) {
            statusProgressBar_->setValue(0);
        }
        statusBar()->showMessage(QStringLiteral("任务队列等待中"));
        return;
    }

    resetExecutionSummary();
    statusBar()->showMessage(QStringLiteral("就绪"));
}

void MainWindow::updateExecuteButtonState() {
    const bool hasSelection = !currentPluginName_.empty() && !currentActionKey_.empty();

    QString error = validateParams();

    if (error.isEmpty() && hasSelection) {
        auto values = paramWidget_->getParamValues();
        auto actionIssue = validateActionSpecificParams(
            currentPluginName_, currentActionKey_, values);
        if (actionIssue.has_value()) {
            error = actionIssue->message;
        }
    }

    auto state = buildExecuteButtonState(hasSelection, error);
    executeButton_->setEnabled(state.enabled);
    executeButton_->setToolTip(state.tooltip);

    if (state.enabled) {
        const int queued = TaskRunner::instance().queuedCount();
        const bool running = TaskRunner::instance().isRunning();
        if (running && queued > 0) {
            executeButton_->setToolTip(
                QStringLiteral("参数就绪，点击后将加入队列（当前排队 %1 个）").arg(queued));
        } else if (running) {
            executeButton_->setToolTip(QStringLiteral("参数就绪，点击后将自动加入队列"));
        } else {
            executeButton_->setToolTip(QStringLiteral("参数就绪，点击执行"));
        }
    }
}

QString MainWindow::validateParams() const {
    if (!paramWidget_) return QStringLiteral("参数面板未初始化");

    auto specs = buildEffectiveParamSpecs(currentPluginName_, currentActionKey_);
    auto values = paramWidget_->getParamValues();

    for (const auto& spec : specs) {
        if (!spec.required) continue;

        auto it = values.find(spec.key);
        if (it == values.end()) {
            return QStringLiteral("缺少必填参数: %1").arg(QString::fromStdString(spec.displayName));
        }

        if (spec.type == ParamType::FilePath || spec.type == ParamType::DirPath) {
            auto* str = std::get_if<std::string>(&it->second);
            if (!str || str->empty()) {
                return QStringLiteral("必填参数不能为空: %1").arg(QString::fromStdString(spec.displayName));
            }

            bool isOutput = spec.key.find("output") != std::string::npos;
            if (!isOutput) {
                QFileInfo fi(QString::fromStdString(*str));
                if (!fi.exists()) {
                    return QStringLiteral("文件不存在: %1").arg(QString::fromStdString(spec.displayName));
                }
            } else {
                QFileInfo fi(QString::fromStdString(*str));
                QFileInfo parentDir(fi.absolutePath());
                if (!parentDir.exists() || !parentDir.isDir()) {
                    return QStringLiteral("输出目录不存在: %1").arg(QString::fromStdString(spec.displayName));
                }
            }
        }

        if (spec.type == ParamType::FilePath && spec.key.find("model") != std::string::npos) {
            auto* str = std::get_if<std::string>(&it->second);
            if (str && !str->empty()) {
                QFileInfo fi(QString::fromStdString(*str));
                if (!fi.exists()) {
                    return QStringLiteral("模型文件不存在: %1").arg(QString::fromStdString(spec.displayName));
                }
            }
        }
    }

    if (batchCheckBox_ && batchCheckBox_->isChecked()) {
        QString batchDir = batchDirEdit_->text().trimmed();
        if (batchDir.isEmpty()) {
            return QStringLiteral("批量处理模式需要指定输入目录");
        }
        QFileInfo fi(batchDir);
        if (!fi.exists() || !fi.isDir()) {
            return QStringLiteral("批量输入目录不存在");
        }
    }

    return {};
}

void MainWindow::updateBatchCount() {
    if (!batchCheckBox_ || !batchCheckBox_->isChecked()) {
        batchCountLabel_->clear();
        return;
    }

    QString dirPath = batchDirEdit_->text().trimmed();
    if (dirPath.isEmpty()) {
        batchCountLabel_->setText(QStringLiteral("0 个文件"));
        return;
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        batchCountLabel_->setText(QStringLiteral("目录不存在"));
        return;
    }

    QString pattern = batchFilterEdit_->text().trimmed();
    if (pattern.isEmpty()) pattern = QStringLiteral("*.tif");

    QStringList nameFilters;
    nameFilters << pattern;

    QStringList files = dir.entryList(nameFilters, QDir::Files);
    batchCountLabel_->setText(QStringLiteral("%1 个文件").arg(files.size()));
}

void MainWindow::onExecuteClicked() {
    if (currentPluginName_.empty() || currentActionKey_.empty()) return;

    if (!paramWidget_->validate()) {
        resultSummaryLabel_->setText(QStringLiteral("参数验证失败，请检查必填参数。"));
        auto values = paramWidget_->getParamValues();
        auto specs = buildEffectiveParamSpecs(currentPluginName_, currentActionKey_);
        for (const auto& spec : specs) {
            if (!spec.required) continue;
            auto it = values.find(spec.key);
            if (it == values.end()) {
                paramWidget_->setHighlightedParam(spec.key);
            } else if (auto* str = std::get_if<std::string>(&it->second); str && str->empty()) {
                paramWidget_->setHighlightedParam(spec.key);
            }
        }
        return;
    }

    const bool hadRunningTask = TaskRunner::instance().isRunning();
    auto params = paramWidget_->getParamValues();

    if (!hadRunningTask) {
        QStringList existingOutputs;
        for (const auto& [key, value] : params) {
            if (key.find("output") == std::string::npos) continue;
            auto* str = std::get_if<std::string>(&value);
            if (!str || str->empty()) continue;
            if (QFileInfo::exists(QString::fromStdString(*str))) {
                existingOutputs << QString::fromStdString(*str);
            }
        }
        if (!existingOutputs.isEmpty()) {
            QMessageBox::StandardButton reply = QMessageBox::warning(
                this,
                QStringLiteral("文件覆盖确认"),
                QStringLiteral("以下输出文件已存在，是否覆盖？\n\n%1\n\n点击「是」覆盖，点击「否」取消执行。")
                    .arg(existingOutputs.join(QStringLiteral("\n"))),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return;
            }
        }
    }

    QString pluginDisp = QString::fromStdString(pluginDisplayName(currentPluginName_));
    auto uiConfig = getActionUiConfig(currentPluginName_, currentActionKey_);
    QString actionDisp = uiConfig.displayName.isEmpty()
        ? QString::fromStdString(currentActionKey_)
        : uiConfig.displayName;

    if (!hadRunningTask && !progressDialog_) {
        ensureProgressDialog();
    }

    if (batchCheckBox_ && batchCheckBox_->isChecked()) {
        const QDir batchDir(batchDirEdit_->text().trimmed());
        QString pattern = batchFilterEdit_->text().trimmed();
        if (pattern.isEmpty()) {
            pattern = QStringLiteral("*.tif");
        }
        const QStringList files = batchDir.entryList(QStringList{pattern}, QDir::Files);
        int submittedCount = 0;
        for (const QString& fileName : files) {
            auto taskParams = params;
            const QString inputPath = batchDir.absoluteFilePath(fileName);
            if (paramWidget_->hasParam("input_raster")) {
                taskParams["input_raster"] = inputPath.toStdString();
            }
            if (paramWidget_->hasParam("output_tif")) {
                taskParams["output_tif"] = buildSuggestedOutputPath(inputPath, QStringLiteral(".tif")).toStdString();
            }
            if (paramWidget_->hasParam("output_shp")) {
                taskParams["output_shp"] = buildSuggestedOutputPath(inputPath, QStringLiteral(".shp")).toStdString();
            }
            const QString taskId = TaskRunner::instance().run(
                QString::fromStdString(currentDisplayGroupKey_),
                QString::fromStdString(currentPluginName_),
                QString::fromStdString(currentActionKey_),
                taskParams,
                pluginDisp,
                actionDisp);
            if (!taskId.isEmpty()) {
                pendingResultTaskIds_.insert(taskId, QString::fromStdString(currentDisplayGroupKey_));
                if (!hadRunningTask && currentTaskId_.isEmpty()) {
                    currentTaskId_ = taskId;
                }
                ++submittedCount;
            }
        }
        if (submittedCount == 0) {
            resultSummaryLabel_->setText(QStringLiteral("批量任务提交失败。"));
            if (progressDialog_) {
                progressDialog_->deleteLater();
                progressDialog_ = nullptr;
            }
            return;
        }
        if (hadRunningTask) {
            showQueuedBatchSummary(submittedCount);
        } else {
            resultSummaryLabel_->setText(QStringLiteral("已提交 %1 个批量任务。").arg(submittedCount));
        }
    } else {
        const QString submittedTaskId = TaskRunner::instance().run(
            QString::fromStdString(currentDisplayGroupKey_),
            QString::fromStdString(currentPluginName_),
            QString::fromStdString(currentActionKey_),
            params,
            pluginDisp,
            actionDisp);
        if (submittedTaskId.isEmpty()) {
            resultSummaryLabel_->setText(QStringLiteral("任务提交失败。"));
            if (progressDialog_) {
                progressDialog_->deleteLater();
                progressDialog_ = nullptr;
            }
            return;
        }
        pendingResultTaskIds_.insert(submittedTaskId, QString::fromStdString(currentDisplayGroupKey_));
        if (!hadRunningTask) {
            currentTaskId_ = submittedTaskId;
            showRunningTaskSummary(currentTaskId_);
            if (progressDialog_) {
                progressDialog_->show();
                progressDialog_->raise();
                progressDialog_->activateWindow();
            }
        } else {
            showQueuedTaskSummary(submittedTaskId);
        }
    }
    updateExecuteButtonState();
}

void MainWindow::onTaskRunnerStarted(const QString& displayGroup, const QString& taskId) {
    const bool affectsCurrentGroup =
        displayGroup == QString::fromStdString(currentDisplayGroupKey_);
    if (currentTaskId_.isEmpty() && pendingResultTaskIds_.contains(taskId)) {
        currentTaskId_ = taskId;
        if (affectsCurrentGroup) {
            ensureProgressDialog();
            showRunningTaskSummary(currentTaskId_);
            if (progressDialog_) {
                progressDialog_->show();
                progressDialog_->raise();
                progressDialog_->activateWindow();
            }
        }
    }
}

void MainWindow::onTaskRunnerFinished(const QString& displayGroup,
                                      const QString& taskId,
                                      bool success,
                                      bool cancelled) {
    if (!pendingResultTaskIds_.contains(taskId)) {
        return;
    }
    const QString taskGroup = pendingResultTaskIds_.value(taskId);
    const bool affectsCurrentGroup =
        taskGroup == QString::fromStdString(currentDisplayGroupKey_);
    pendingResultTaskIds_.remove(taskId);

    const auto rec = TaskManager::instance().findTask(displayGroup, taskId);
    const QString message = rec.resultMessage.isEmpty()
        ? (success ? QStringLiteral("执行成功") : QStringLiteral("执行失败"))
        : rec.resultMessage;

    lastExecutionSuccess_ = success;
    lastExecutionCancelled_ = cancelled;
    lastExecutionMessage_ = message;
    lastExecutionRawMessage_ = rec.resultRawMessage.isEmpty() ? message : rec.resultRawMessage;

    if (taskId == currentTaskId_) {
        if (progressDialog_ && affectsCurrentGroup) {
            progressDialog_->setFinished(message, success, cancelled);
        }
        if (affectsCurrentGroup) {
            showFinishedTaskSummary(taskId, message, success, cancelled);
        }
        currentTaskId_.clear();
        if (!affectsCurrentGroup) {
            syncExecutionSummaryForCurrentGroup();
        }
        emit executionFinished(success);
    }

    updateExecuteButtonState();
}

void MainWindow::onTaskProgressChanged(const QString& taskId, double percent) {
    const bool affectsCurrentGroup =
        pendingResultTaskIds_.value(taskId) == QString::fromStdString(currentDisplayGroupKey_);
    if (!affectsCurrentGroup && taskId != currentTaskId_) {
        taskCenterPage_->updateTaskProgress(taskId, percent);
        return;
    }

    int value = std::clamp(static_cast<int>(percent * 100.0), 0, 100);
    if (affectsCurrentGroup) {
        statusProgressBar_->setValue(value);
    }

    if (progressDialog_ && affectsCurrentGroup) {
        progressDialog_->updateProgress(percent);
    }

    taskCenterPage_->updateTaskProgress(taskId, percent);
}

void MainWindow::onTaskLogMessage(const QString& displayGroup, const QString& taskId, const QString& msg) {
    const bool affectsCurrentGroup =
        displayGroup == QString::fromStdString(currentDisplayGroupKey_);
    if (taskId == currentTaskId_ && affectsCurrentGroup) {
        if (progressDialog_) {
            progressDialog_->appendLog(msg);
        }
    }
    taskCenterPage_->appendLog(taskId, msg);
}

void MainWindow::onRerunTask(const QString& taskId) {
    const QString displayGroup = taskCenterPage_ ? taskCenterPage_->currentGroup() : QStringLiteral("default");
    auto rec = TaskManager::instance().findTask(displayGroup, taskId);
    if (rec.id.isEmpty()) return;

    currentDisplayGroupKey_ = rec.displayGroup.isEmpty()
        ? rec.pluginName.toStdString()
        : rec.displayGroup.toStdString();
    currentPluginName_ = rec.pluginName.toStdString();
    currentActionKey_ = rec.actionKey.toStdString();

    navPanel_->setCurrentPluginSelection(currentPluginName_);
    navPanel_->setCurrentSubFunctionSelection(currentPluginName_, currentActionKey_);

    auto specs = buildEffectiveParamSpecs(currentPluginName_, currentActionKey_);
    paramWidget_->setParamSpecs(specs);

    for (const auto& [key, value] : rec.params) {
        if (auto* str = std::get_if<std::string>(&value)) {
            paramWidget_->setStringValue(key, *str);
        } else if (auto* ext = std::get_if<std::array<double, 4>>(&value)) {
            paramWidget_->setExtentValue(key, *ext);
        } else {
            paramWidget_->setValueFromString(key, "");
        }
    }

    auto uiConfig = getActionUiConfig(currentPluginName_, currentActionKey_);
    const QString pluginDisplay = QString::fromStdString(pluginDisplayName(currentPluginName_));
    const QString actionDisplay = uiConfig.displayName.isEmpty()
        ? QString::fromStdString(currentActionKey_)
        : uiConfig.displayName;

    applyFunctionPanelState(
        QStringLiteral("%1 / %2").arg(pluginDisplay, actionDisplay),
        uiConfig.description,
        QStringLiteral("当前状态：已加载历史参数，可修改后执行"),
        QStringLiteral("当前算法：%1 / %2").arg(pluginDisplay, actionDisplay),
        currentPluginName_);
    resultSummaryLabel_->setText(
        QStringLiteral("已载入任务 %1 的历史参数，可直接执行或修改后执行。").arg(taskId));
    if (taskCenterPage_) {
        taskCenterPage_->setCurrentGroup(displayGroup);
    }

    tabWidget_->setCurrentIndex(0);
    updateExecuteButtonState();
}

void MainWindow::onEditTask(const QString& taskId) {
    onRerunTask(taskId);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    if (!event->mimeData()->hasUrls()) return;
    event->acceptProposedAction();

    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    QString filePath = urls.first().toLocalFile();
    if (filePath.isEmpty()) return;

    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();

    if (suffix == QStringLiteral("tif") || suffix == QStringLiteral("tiff") ||
        suffix == QStringLiteral("img") || suffix == QStringLiteral("png")) {
        if (paramWidget_ && paramWidget_->hasParam("input_raster")) {
            paramWidget_->setStringValue("input_raster", filePath.toStdString());
        }
    } else if (suffix == QStringLiteral("shp") || suffix == QStringLiteral("geojson") ||
               suffix == QStringLiteral("gpkg")) {
        if (paramWidget_ && paramWidget_->hasParam("input_vector")) {
            paramWidget_->setStringValue("input_vector", filePath.toStdString());
        }
    }
}

}
