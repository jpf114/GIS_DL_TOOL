#pragma once

#include <QMainWindow>
#include <QMap>
#include <QString>
#include <map>
#include <string>
#include <array>
#include <optional>
#include "param_card_widget.h"

class QFrame;
class QLabel;
class QPushButton;
class QTabWidget;
class QProgressBar;
class QScrollArea;
class QDragEnterEvent;
class QDropEvent;
class QThread;
class QCheckBox;
class QLineEdit;

namespace gis_ai::gui {

class NavPanel;
class ParamWidget;
class TaskCenterPage;
class ExecuteWorker;
class QtProgressReporter;
class ProgressDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    void selectPluginByName(const std::string& pluginName);
    void selectActionByKey(const std::string& actionKey);
    bool setParamValue(const std::string& key, const std::string& value);
    bool setBatchMode(bool enabled);
    bool setBatchInputDirectory(const std::string& directory);
    bool setBatchFilter(const std::string& filter);
    void triggerExecute();
    bool lastExecutionSuccess() const;
    bool lastExecutionCancelled() const;
    QString lastExecutionMessage() const;
    QString lastExecutionRawMessage() const;

signals:
    void executionFinished(bool success);

private:
    void setupUi();

    void onPluginSelected(const std::string& pluginName);
    void onSubFunctionSelected(const std::string& pluginName, const std::string& actionKey);
    void onExecuteClicked();
    void onTaskRunnerStarted(const QString& displayGroup, const QString& taskId);
    void onParamsChanged();
    void onTaskRunnerFinished(const QString& displayGroup, const QString& taskId,
                              bool success, bool cancelled);
    void onTaskProgressChanged(const QString& taskId, double percent);
    void onTaskLogMessage(const QString& displayGroup, const QString& taskId, const QString& msg);
    void onRerunTask(const QString& taskId);
    void onEditTask(const QString& taskId);

    void syncDerivedParams();
    void ensureProgressDialog();
    void resetExecutionSummary();
    bool currentGroupHasPendingExecution() const;
    void discardTaskExecutionState(const QStringList& taskIds);
    void showRunningTaskSummary(const QString& taskId);
    void showQueuedTaskSummary(const QString& taskId);
    void showQueuedBatchSummary(int submittedCount);
    void showFinishedTaskSummary(const QString& taskId,
                                 const QString& message,
                                 bool success,
                                 bool cancelled);
    void applyFunctionPanelState(const QString& title,
                                 const QString& description,
                                 const QString& metaText,
                                 const QString& algorithmText,
                                 const std::string& pluginName);
    void syncExecutionSummaryForCurrentGroup();
    void updateExecuteButtonState();
    QString validateParams() const;
    void updateBatchCount();

    QString buildSuggestedOutputPath(const QString& inputPath,
                                     const QString& ext) const;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    NavPanel* navPanel_ = nullptr;
    ParamWidget* paramWidget_ = nullptr;
    TaskCenterPage* taskCenterPage_ = nullptr;
    QScrollArea* paramScrollArea_ = nullptr;
    QLabel* functionIconLabel_ = nullptr;
    QLabel* functionTitleLabel_ = nullptr;
    QLabel* functionDescLabel_ = nullptr;
    QLabel* functionMetaLabel_ = nullptr;
    QPushButton* executeButton_ = nullptr;
    QLabel* resultSummaryLabel_ = nullptr;
    QLabel* statusAlgorithmLabel_ = nullptr;
    QProgressBar* statusProgressBar_ = nullptr;

    QTabWidget* tabWidget_ = nullptr;

    QCheckBox* batchCheckBox_ = nullptr;
    QLineEdit* batchDirEdit_ = nullptr;
    QPushButton* batchDirButton_ = nullptr;
    QLineEdit* batchFilterEdit_ = nullptr;
    QLabel* batchCountLabel_ = nullptr;

    ProgressDialog* progressDialog_ = nullptr;

    std::string currentPluginName_;
    std::string currentActionKey_;
    QString currentTaskId_;
    QMap<QString, QString> pendingResultTaskIds_;

    std::map<std::string, QString> lastAutoOutputPath_;
    std::map<std::string, std::array<double, 4>> lastAutoExtent_;
    std::string currentDisplayGroupKey_{"default"};
    bool lastExecutionSuccess_ = false;
    bool lastExecutionCancelled_ = false;
    QString lastExecutionMessage_;
    QString lastExecutionRawMessage_;
};

}
