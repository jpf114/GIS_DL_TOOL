#pragma once

#include <QMainWindow>
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

namespace gis_ai::gui {

class NavPanel;
class ParamWidget;
class TaskCenterPage;
class ExecutionController;
class BatchModeController;

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
    void connectControllerSignals();

    void onPluginSelected(const std::string& pluginName);
    void onSubFunctionSelected(const std::string& pluginName, const std::string& actionKey);
    void onExecuteClicked();
    void onParamsChanged();

    void syncDerivedParams();
    void resetExecutionSummary();
    void syncExecutionSummaryForCurrentGroup();
    void updateExecuteButtonState();
    QString validateParams() const;

    void showRunningTaskSummary();
    void showQueuedTaskSummary();
    void showQueuedBatchSummary(int submittedCount);
    void showFinishedTaskSummary(const QString& message, bool success, bool cancelled);

    void applyFunctionPanelState(const QString& title,
                                 const QString& description,
                                 const QString& metaText,
                                 const QString& algorithmText,
                                 const std::string& pluginName);

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

    ExecutionController* execController_ = nullptr;
    BatchModeController* batchController_ = nullptr;

    std::string currentPluginName_;
    std::string currentActionKey_;

    std::map<std::string, QString> lastAutoOutputPath_;
    std::map<std::string, std::array<double, 4>> lastAutoExtent_;
};

}
