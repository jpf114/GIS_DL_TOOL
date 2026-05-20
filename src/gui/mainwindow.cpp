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
#include "data_inspector.h"
#include "action_dispatcher.h"
#include "execution_controller.h"
#include "batch_mode_controller.h"
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

QString suffixForAction(const std::string& pluginName, const std::string& actionKey) {
    Q_UNUSED(actionKey);
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
    connectControllerSignals();
    restoreWindowState();
}

MainWindow::~MainWindow() {
    saveWindowState();
}

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
    if (!isKnownAction(currentPluginName_, actionKey)) {
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
    return batchController_->setBatchMode(enabled);
}

bool MainWindow::setBatchInputDirectory(const std::string& directory) {
    return batchController_->setBatchInputDirectory(directory);
}

bool MainWindow::setBatchFilter(const std::string& filter) {
    return batchController_->setBatchFilter(filter);
}

void MainWindow::triggerExecute() {
    onExecuteClicked();
}

bool MainWindow::lastExecutionSuccess() const {
    return execController_->lastSuccess();
}

bool MainWindow::lastExecutionCancelled() const {
    return execController_->lastCancelled();
}

QString MainWindow::lastExecutionMessage() const {
    return execController_->lastMessage();
}

QString MainWindow::lastExecutionRawMessage() const {
    return execController_->lastRawMessage();
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

    execController_ = new ExecutionController(this);
    batchController_ = new BatchModeController(this);

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

    QWidget* batchWidget = batchController_->createWidget(this);
    executionLayout->addWidget(batchWidget);

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
    taskCenterPage_->setCurrentGroup(
        QString::fromStdString(execController_->displayGroupKey()));
    tabWidget_->addTab(taskCenterPage_, QStringLiteral("任务中心"));

    connect(taskCenterPage_, &TaskCenterPage::clearHistoryRequested, this, [this]() {
        const QString displayGroup = taskCenterPage_->currentGroup();
        QStringList taskIds;
        const auto tasks = TaskManager::instance().recentTasks(displayGroup, 1000);
        taskIds.reserve(tasks.size());
        for (const auto& task : tasks) {
            taskIds.push_back(task.id);
        }
        execController_->discardTaskState(taskIds);
        TaskManager::instance().clearHistory(taskCenterPage_->currentGroup());
        taskCenterPage_->refreshAll();
        resetExecutionSummary();
    });

    connect(taskCenterPage_, &TaskCenterPage::deleteTasksRequested, this, [this](const QStringList& ids) {
        const QString displayGroup = taskCenterPage_->currentGroup();
        execController_->discardTaskState(ids);
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

    connect(taskCenterPage_, &TaskCenterPage::rerunTaskRequested, this, [this](const QString& taskId) {
        const QString displayGroup = taskCenterPage_ ? taskCenterPage_->currentGroup() : QStringLiteral("default");
        auto rec = TaskManager::instance().findTask(displayGroup, taskId);
        if (rec.id.isEmpty()) return;

        execController_->setDisplayGroupKey(rec.displayGroup.isEmpty()
            ? rec.pluginName.toStdString()
            : rec.displayGroup.toStdString());
        currentPluginName_ = rec.pluginName.toStdString();
        currentActionKey_ = rec.actionKey.toStdString();
        batchController_->updateVisibility(currentPluginName_, currentActionKey_);

        navPanel_->setCurrentPluginSelection(currentPluginName_);
        navPanel_->setCurrentSubFunctionSelection(currentPluginName_, currentActionKey_);

        auto specs = buildEffectiveParamSpecs(currentPluginName_, currentActionKey_);
        paramWidget_->setUiContext(currentPluginName_, currentActionKey_);
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
    });

    connect(taskCenterPage_, &TaskCenterPage::editTaskRequested,
            taskCenterPage_, &TaskCenterPage::rerunTaskRequested);

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

void MainWindow::connectControllerSignals() {
    connect(execController_, &ExecutionController::activeTaskStarted,
            this, [this](const QString& taskId, bool affectsCurrentGroup) {
        Q_UNUSED(taskId);
        if (affectsCurrentGroup) {
            execController_->ensureProgressDialog(this);
            showRunningTaskSummary();
            if (execController_->progressDialog()) {
                execController_->progressDialog()->show();
                execController_->progressDialog()->raise();
                execController_->progressDialog()->activateWindow();
            }
        }
    });

    connect(execController_, &ExecutionController::taskCompleted,
            this, [this](const QString& taskId, bool affectsCurrentGroup,
                         bool success, bool cancelled,
                         const QString& message, const QString& rawMessage,
                         bool wasCurrentTask) {
        Q_UNUSED(rawMessage);
        if (wasCurrentTask) {
            if (execController_->progressDialog() && affectsCurrentGroup) {
                execController_->progressDialog()->setFinished(message, success, cancelled);
            }
            if (affectsCurrentGroup) {
                showFinishedTaskSummary(message, success, cancelled);
            } else {
                syncExecutionSummaryForCurrentGroup();
            }
            emit executionFinished(success);
        }
        updateExecuteButtonState();
    });

    connect(execController_, &ExecutionController::progressChanged,
            this, [this](const QString& taskId, bool affectsCurrentGroup, double percent) {
        if (!affectsCurrentGroup && taskId != execController_->currentTaskId()) {
            taskCenterPage_->updateTaskProgress(taskId, percent);
            return;
        }

        int value = std::clamp(static_cast<int>(percent * 100.0), 0, 100);
        if (affectsCurrentGroup) {
            statusProgressBar_->setValue(value);
        }

        if (execController_->progressDialog() && affectsCurrentGroup) {
            execController_->progressDialog()->updateProgress(percent);
        }

        taskCenterPage_->updateTaskProgress(taskId, percent);
    });

    connect(execController_, &ExecutionController::logMessage,
            this, [this](const QString& taskId, bool affectsCurrentGroup, const QString& msg) {
        if (taskId == execController_->currentTaskId() && affectsCurrentGroup) {
            if (execController_->progressDialog()) {
                execController_->progressDialog()->appendLog(msg);
            }
        }
        taskCenterPage_->appendLog(taskId, msg);
    });

    connect(execController_, &ExecutionController::activeTaskDiscarded,
            this, [this]() {
        resetExecutionSummary();
        statusBar()->showMessage(QStringLiteral("就绪"));
    });

    connect(execController_, &ExecutionController::executionFinished,
            this, &MainWindow::executionFinished);

    connect(batchController_, &BatchModeController::stateChanged,
            this, &MainWindow::updateExecuteButtonState);
}

void MainWindow::onPluginSelected(const std::string& pluginName) {
    currentPluginName_.clear();
    currentActionKey_.clear();
    lastAutoOutputPath_.clear();
    const std::string groupKey = pluginName.empty() ? std::string{"default"} : pluginName;
    execController_->setDisplayGroupKey(groupKey);
    batchController_->updateVisibility(currentPluginName_, currentActionKey_);

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
            taskCenterPage_->setCurrentGroup(
                QString::fromStdString(execController_->displayGroupKey()));
        }
        syncExecutionSummaryForCurrentGroup();
        statusBar()->showMessage(QStringLiteral("就绪"));
        return;
    }

    currentPluginName_ = pluginName;
    batchController_->updateVisibility(currentPluginName_, currentActionKey_);
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
        taskCenterPage_->setCurrentGroup(
            QString::fromStdString(execController_->displayGroupKey()));
    }
    syncExecutionSummaryForCurrentGroup();
    if (!execController_->hasPendingExecution() && resultSummaryLabel_) {
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
    execController_->setDisplayGroupKey(pluginName.empty() ? std::string{"default"} : pluginName);
    batchController_->updateVisibility(currentPluginName_, currentActionKey_);

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
    paramWidget_->setUiContext(pluginName, actionKey);
    paramWidget_->setParamSpecs(specs);
    if (taskCenterPage_) {
        taskCenterPage_->setCurrentGroup(
            QString::fromStdString(execController_->displayGroupKey()));
    }
    syncExecutionSummaryForCurrentGroup();
    if (!execController_->hasPendingExecution() && resultSummaryLabel_) {
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

void MainWindow::resetExecutionSummary() {
    if (resultSummaryLabel_) {
        resultSummaryLabel_->setText(
            QStringLiteral("当前未执行任务。选择子功能并补全参数后，可以直接开始运行。"));
    }
    if (statusProgressBar_) {
        statusProgressBar_->setValue(0);
    }
}

void MainWindow::syncExecutionSummaryForCurrentGroup() {
    if (!execController_->currentTaskId().isEmpty() && execController_->hasPendingExecution()) {
        showRunningTaskSummary();
        return;
    }

    int queuedCount = execController_->pendingCountForCurrentGroup();
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

    if (batchController_->genericBatchModeEnabled()) {
        QString batchDir = batchController_->batchDir();
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

void MainWindow::showRunningTaskSummary() {
    if (resultSummaryLabel_) {
        resultSummaryLabel_->setStyleSheet(QString());
        resultSummaryLabel_->setText(QStringLiteral("正在执行，请稍候..."));
    }
    if (statusProgressBar_) {
        statusProgressBar_->setValue(0);
    }
    statusBar()->showMessage(QStringLiteral("执行中"));
}

void MainWindow::showQueuedTaskSummary() {
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

void MainWindow::showFinishedTaskSummary(const QString& message,
                                          bool success,
                                          bool cancelled) {
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
    if (!currentActionKey_.empty() && mgr.hasActionIcon(pluginName, currentActionKey_)) {
        functionIconLabel_->setPixmap(mgr.pixmapForAction(pluginName, currentActionKey_, 38, QColor("#2F7CF6")));
    } else if (!pluginName.empty() && mgr.hasPluginIcon(pluginName)) {
        functionIconLabel_->setPixmap(mgr.pixmapForPlugin(pluginName, 38, QColor("#2F7CF6")));
    } else {
        functionIconLabel_->setPixmap(defaultBadgePixmap());
    }
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

    if (!hadRunningTask && !execController_->progressDialog()) {
        execController_->ensureProgressDialog(this);
    }

    const QString group = QString::fromStdString(execController_->displayGroupKey());

    if (batchController_->genericBatchModeEnabled()) {
        const QDir batchDir(batchController_->batchDir());
        const QStringList files = batchController_->batchFiles();
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
                group,
                QString::fromStdString(currentPluginName_),
                QString::fromStdString(currentActionKey_),
                taskParams,
                pluginDisp,
                actionDisp);
            if (!taskId.isEmpty()) {
                execController_->trackTask(taskId, group, hadRunningTask);
                ++submittedCount;
            }
        }
        if (submittedCount == 0) {
            resultSummaryLabel_->setText(QStringLiteral("批量任务提交失败。"));
            execController_->destroyProgressDialog();
            return;
        }
        if (hadRunningTask) {
            showQueuedBatchSummary(submittedCount);
        } else {
            resultSummaryLabel_->setText(QStringLiteral("已提交 %1 个批量任务。").arg(submittedCount));
        }
    } else {
        const QString submittedTaskId = TaskRunner::instance().run(
            group,
            QString::fromStdString(currentPluginName_),
            QString::fromStdString(currentActionKey_),
            params,
            pluginDisp,
            actionDisp);
        if (submittedTaskId.isEmpty()) {
            resultSummaryLabel_->setText(QStringLiteral("任务提交失败。"));
            execController_->destroyProgressDialog();
            return;
        }
        execController_->trackTask(submittedTaskId, group, hadRunningTask);
        if (!hadRunningTask) {
            showRunningTaskSummary();
            if (execController_->progressDialog()) {
                execController_->progressDialog()->show();
                execController_->progressDialog()->raise();
                execController_->progressDialog()->activateWindow();
            }
        } else {
            showQueuedTaskSummary();
        }
    }
    updateExecuteButtonState();
}

void MainWindow::saveWindowState() {
    auto& s = SettingsManager::instance();
    s.setWindowGeometry(saveGeometry());
    s.setWindowState(saveState());
    if (!currentPluginName_.empty()) {
        s.setLastPluginName(QString::fromStdString(currentPluginName_));
    }
    if (!currentActionKey_.empty()) {
        s.setLastActionKey(QString::fromStdString(currentActionKey_));
    }
}

void MainWindow::restoreWindowState() {
    auto& s = SettingsManager::instance();
    const QByteArray geometry = s.windowGeometry();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    const QByteArray state = s.windowState();
    if (!state.isEmpty()) {
        restoreState(state);
    }
    const QString lastPlugin = s.lastPluginName();
    const QString lastAction = s.lastActionKey();
    if (!lastPlugin.isEmpty()) {
        selectPluginByName(lastPlugin.toStdString());
        if (!lastAction.isEmpty()) {
            selectActionByKey(lastAction.toStdString());
        }
    }
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
