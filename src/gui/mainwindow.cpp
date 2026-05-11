#include "mainwindow.h"
#include "style_constants.h"
#include "nav_panel.h"
#include "icon_manager.h"
#include "param_widget.h"
#include "task_center_page.h"
#include "task_manager.h"
#include "settings_manager.h"
#include "gdal_config.h"
#include "gui_data_support.h"
#include "execute_worker.h"
#include "qt_progress_reporter.h"
#include "progress_dialog.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QStatusBar>
#include <QTabWidget>
#include <QThread>
#include <QVBoxLayout>

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

}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setAcceptDrops(true);
    setupUi();
}

MainWindow::~MainWindow() {
    if (workerThread_ && workerThread_->isRunning()) {
        workerThread_->quit();
        workerThread_->wait();
    }
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("GIS AI 工具台"));
    resize(Size::kWindowDefaultWidth, Size::kWindowDefaultHeight);
    setMinimumSize(Size::kWindowMinWidth, Size::kWindowMinHeight);
    setStyleSheet(globalStyleSheet());

    QDir appDir(QApplication::applicationDirPath());
    QString iconsBasePath = appDir.absoluteFilePath(QStringLiteral("../resources/icons"));
    if (!QDir(iconsBasePath).exists()) {
        iconsBasePath = QDir(QStringLiteral("resources/icons")).absolutePath();
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
    functionMetaLabel_->setObjectName(QStringLiteral("heroMeta"));
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
    connect(executeButton_, &QPushButton::clicked, this, &MainWindow::onExecuteClicked);
    execHeaderLayout->addWidget(executeButton_);

    executionLayout->addLayout(execHeaderLayout);

    resultSummaryLabel_ = new QLabel;
    resultSummaryLabel_->setWordWrap(true);
    resultSummaryLabel_->setObjectName(QStringLiteral("execSummary"));
    resultSummaryLabel_->setMinimumHeight(28);
    resultSummaryLabel_->setText(
        QStringLiteral("当前未执行任务。选择子功能并补全参数后，可以直接开始运行。"));
    executionLayout->addWidget(resultSummaryLabel_);

    rightLayout->addWidget(executionCard);

    tabWidget_ = new QTabWidget;
    tabWidget_->setObjectName(QStringLiteral("pagePanel"));
    tabWidget_->setTabPosition(QTabWidget::North);
    tabWidget_->addTab(rightPanel, QStringLiteral("功能配置"));

    taskCenterPage_ = new TaskCenterPage;
    taskCenterPage_->setCurrentGroup(QStringLiteral("default"));
    tabWidget_->addTab(taskCenterPage_, QStringLiteral("任务中心"));

    connect(taskCenterPage_, &TaskCenterPage::clearHistoryRequested, this, [this]() {
        TaskManager::instance().clearHistory(QStringLiteral("default"));
        taskCenterPage_->refreshAll();
    });

    connect(taskCenterPage_, &TaskCenterPage::deleteTasksRequested, this, [this](const QStringList& ids) {
        TaskManager::instance().deleteTasks(QStringLiteral("default"), ids);
        taskCenterPage_->removeTaskRows(ids);
    });

    connect(taskCenterPage_, &TaskCenterPage::clearLogsRequested, this, [this](const QString& taskId) {
        TaskManager::instance().appendLog(QStringLiteral("default"), taskId, QStringLiteral("日志已清空"));
        taskCenterPage_->clearLogDisplay();
    });

    connect(taskCenterPage_, &TaskCenterPage::rerunTaskRequested, this, &MainWindow::onRerunTask);
    connect(taskCenterPage_, &TaskCenterPage::editTaskRequested, this, &MainWindow::onEditTask);

    connect(&TaskManager::instance(), &TaskManager::taskSubmitted, this,
            [this](const QString& group, const QString& id) {
        if (group != QStringLiteral("default")) return;
        auto rec = TaskManager::instance().findTask(group, id);
        taskCenterPage_->addTaskRow(id, rec.actionDisplayName,
            static_cast<int>(rec.status),
            rec.startTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    });

    connect(&TaskManager::instance(), &TaskManager::taskFinished, this,
            [this](const QString& group, const QString& id) {
        if (group != QStringLiteral("default")) return;
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

void MainWindow::onPluginSelected(const std::string& pluginName) {
    currentPluginName_.clear();
    currentActionKey_.clear();

    if (pluginName.empty()) {
        functionTitleLabel_->setText(QStringLiteral("请选择功能"));
        functionDescLabel_->setText(
            QStringLiteral("从左侧选择主功能和子功能后，这里会显示功能说明和参数配置。"));
        functionMetaLabel_->setText(QStringLiteral("当前状态：等待选择主功能"));
        functionIconLabel_->setPixmap(defaultBadgePixmap());
        executeButton_->setEnabled(false);
        statusAlgorithmLabel_->setText(QStringLiteral("当前算法：未选择"));
        if (paramWidget_) {
            paramWidget_->clear();
        }
        return;
    }

    currentPluginName_ = pluginName;
    const QString displayName = QString::fromStdString(pluginDisplayName(pluginName));
    functionTitleLabel_->setText(displayName);
    functionDescLabel_->setText(
        QStringLiteral("已选择 %1，请选择子功能进行操作。").arg(displayName));
    functionMetaLabel_->setText(QStringLiteral("当前状态：已选择主功能"));
    executeButton_->setEnabled(false);
    statusAlgorithmLabel_->setText(QStringLiteral("当前算法：%1").arg(displayName));

    if (paramWidget_) {
        paramWidget_->clear();
    }

    auto& mgr = IconManager::instance();
    if (mgr.hasPluginIcon(pluginName)) {
        QPixmap iconPixmap = mgr.pixmapForPlugin(pluginName, 38, QColor("#2F7CF6"));
        functionIconLabel_->setPixmap(iconPixmap);
    } else {
        functionIconLabel_->setPixmap(defaultBadgePixmap());
    }
}

void MainWindow::onSubFunctionSelected(const std::string& pluginName,
                                       const std::string& actionKey) {
    currentPluginName_ = pluginName;
    currentActionKey_ = actionKey;

    auto uiConfig = getActionUiConfig(pluginName, actionKey);
    const QString pluginDisplay = QString::fromStdString(pluginDisplayName(pluginName));
    const QString actionDisplay = uiConfig.displayName.isEmpty()
        ? QString::fromStdString(actionKey)
        : uiConfig.displayName;

    functionTitleLabel_->setText(
        QStringLiteral("%1 / %2").arg(pluginDisplay, actionDisplay));
    functionDescLabel_->setText(uiConfig.description);
    functionMetaLabel_->setText(QStringLiteral("当前状态：已选择子功能，可配置参数"));
    statusAlgorithmLabel_->setText(
        QStringLiteral("当前算法：%1 / %2").arg(pluginDisplay, actionDisplay));

    auto& mgr = IconManager::instance();
    if (mgr.hasPluginIcon(pluginName)) {
        QPixmap iconPixmap = mgr.pixmapForPlugin(pluginName, 38, QColor("#2F7CF6"));
        functionIconLabel_->setPixmap(iconPixmap);
    } else {
        functionIconLabel_->setPixmap(defaultBadgePixmap());
    }

    auto specs = buildEffectiveParamSpecs(pluginName, actionKey);
    paramWidget_->setParamSpecs(specs);

    updateExecuteButtonState();
}

void MainWindow::onParamsChanged() {
    syncDerivedParams();
    updateExecuteButtonState();
}

void MainWindow::syncDerivedParams() {
    if (currentPluginName_.empty() || currentActionKey_.empty()) return;

    if (paramWidget_->hasParam("input_raster")) {
        auto inputPath = paramWidget_->stringValue("input_raster");
        if (!inputPath.empty()) {
            QString qInput = QString::fromStdString(inputPath);
            if (paramWidget_->hasParam("output_tif")) {
                auto currentOutput = paramWidget_->stringValue("output_tif");
                if (currentOutput.empty()) {
                    paramWidget_->setStringValue("output_tif",
                        deriveOutputPath(qInput, QStringLiteral("_result.tif")).toStdString());
                }
            }
            if (paramWidget_->hasParam("output_path")) {
                auto currentOutput = paramWidget_->stringValue("output_path");
                if (currentOutput.empty()) {
                    paramWidget_->setStringValue("output_path",
                        deriveOutputPath(qInput, QStringLiteral("_result.tif")).toStdString());
                }
            }
            if (paramWidget_->hasParam("output_shp")) {
                auto currentOutput = paramWidget_->stringValue("output_shp");
                if (currentOutput.empty()) {
                    paramWidget_->setStringValue("output_shp",
                        deriveOutputPath(qInput, QStringLiteral("_result.shp")).toStdString());
                }
            }
        }
    }
}

void MainWindow::updateExecuteButtonState() {
    if (currentPluginName_.empty() || currentActionKey_.empty()) {
        executeButton_->setEnabled(false);
        return;
    }

    bool hasWorker = workerThread_ && workerThread_->isRunning();
    executeButton_->setEnabled(!hasWorker);
}

void MainWindow::onExecuteClicked() {
    if (currentPluginName_.empty() || currentActionKey_.empty()) return;
    if (workerThread_ && workerThread_->isRunning()) return;

    if (!paramWidget_->validate()) {
        resultSummaryLabel_->setText(QStringLiteral("参数验证失败，请检查必填参数。"));
        return;
    }

    auto params = paramWidget_->getParamValues();

    QString pluginDisp = QString::fromStdString(pluginDisplayName(currentPluginName_));
    auto uiConfig = getActionUiConfig(currentPluginName_, currentActionKey_);
    QString actionDisp = uiConfig.displayName.isEmpty()
        ? QString::fromStdString(currentActionKey_)
        : uiConfig.displayName;

    currentTaskId_ = TaskManager::instance().submitTask(
        QStringLiteral("default"),
        QString::fromStdString(currentPluginName_),
        QString::fromStdString(currentActionKey_),
        params,
        pluginDisp,
        actionDisp);

    if (currentTaskId_.isEmpty()) {
        resultSummaryLabel_->setText(QStringLiteral("任务提交失败。"));
        return;
    }

    TaskManager::instance().updateTaskStatus(QStringLiteral("default"), currentTaskId_,
        TaskRecord::Running);

    currentReporter_ = new QtProgressReporter(currentTaskId_);
    connect(currentReporter_, &QtProgressReporter::progressChanged,
            this, &MainWindow::onProgressChanged);
    connect(currentReporter_, &QtProgressReporter::messageLogged,
            this, &MainWindow::onMessageLogged);

    currentWorker_ = new ExecuteWorker;
    currentWorker_->setup(
        QString::fromStdString(currentPluginName_),
        QString::fromStdString(currentActionKey_),
        params,
        currentReporter_);

    workerThread_ = new QThread;
    currentWorker_->moveToThread(workerThread_);

    connect(workerThread_, &QThread::started, currentWorker_, &ExecuteWorker::run);
    connect(currentWorker_, &ExecuteWorker::finished, this, &MainWindow::onWorkerFinished);
    connect(currentWorker_, &ExecuteWorker::finished, workerThread_, &QThread::quit);
    connect(workerThread_, &QThread::finished, workerThread_, &QThread::deleteLater);
    connect(workerThread_, &QThread::finished, this, [this]() {
        workerThread_ = nullptr;
        updateExecuteButtonState();
    });

    progressDialog_ = new ProgressDialog(this);
    connect(progressDialog_, &ProgressDialog::rejected, this, [this]() {
        if (currentReporter_) {
            currentReporter_->cancel();
        }
    });

    executeButton_->setEnabled(false);
    resultSummaryLabel_->setText(
        QStringLiteral("任务 %1 正在执行...").arg(currentTaskId_));

    workerThread_->start();
    progressDialog_->exec();
}

void MainWindow::onWorkerFinished(bool success, const QString& message) {
    if (progressDialog_) {
        bool cancelled = currentReporter_ && currentReporter_->isCancelled();
        progressDialog_->setFinished(message, success, cancelled);
    }

    if (!currentTaskId_.isEmpty()) {
        TaskManager::instance().finishTask(
            QStringLiteral("default"),
            currentTaskId_,
            success,
            currentReporter_ && currentReporter_->isCancelled(),
            message,
            {});

        auto rec = TaskManager::instance().findTask(QStringLiteral("default"), currentTaskId_);
        taskCenterPage_->updateTaskRow(currentTaskId_,
            static_cast<int>(rec.status),
            rec.endTime.toString(Qt::ISODate),
            rec.durationMs);
    }

    if (success) {
        resultSummaryLabel_->setText(
            QStringLiteral("任务 %1 执行成功。%2").arg(currentTaskId_, message));
        statusBar()->showMessage(QStringLiteral("执行完成"));
    } else {
        resultSummaryLabel_->setText(
            QStringLiteral("任务 %1 执行失败。%2").arg(currentTaskId_, message));
        statusBar()->showMessage(QStringLiteral("执行失败"));
    }

    statusProgressBar_->setValue(success ? 100 : 0);

    if (currentWorker_) {
        currentWorker_->deleteLater();
        currentWorker_ = nullptr;
    }
    if (currentReporter_) {
        currentReporter_->deleteLater();
        currentReporter_ = nullptr;
    }

    updateExecuteButtonState();
}

void MainWindow::onProgressChanged(const QString& taskId, double percent) {
    int value = std::clamp(static_cast<int>(percent * 100.0), 0, 100);
    statusProgressBar_->setValue(value);

    if (progressDialog_) {
        progressDialog_->updateProgress(percent);
    }

    if (taskId == currentTaskId_) {
        taskCenterPage_->updateTaskProgress(taskId, percent);
    }
}

void MainWindow::onMessageLogged(const QString& taskId, const QString& msg) {
    if (taskId == currentTaskId_) {
        TaskManager::instance().appendLog(QStringLiteral("default"), taskId, msg);
        taskCenterPage_->appendLog(taskId, msg);

        if (progressDialog_) {
            progressDialog_->appendLog(msg);
        }
    }
}

void MainWindow::onRerunTask(const QString& taskId) {
    auto rec = TaskManager::instance().findTask(QStringLiteral("default"), taskId);
    if (rec.id.isEmpty()) return;

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

    functionTitleLabel_->setText(
        QStringLiteral("%1 / %2").arg(pluginDisplay, actionDisplay));
    functionDescLabel_->setText(uiConfig.description);
    functionMetaLabel_->setText(QStringLiteral("当前状态：已加载历史参数，可修改后执行"));

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
