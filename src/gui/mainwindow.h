#pragma once

#include <QMainWindow>
#include <QString>

class QFrame;
class QLabel;
class QPushButton;
class QTabWidget;
class QProgressBar;
class QScrollArea;
class QDragEnterEvent;
class QDropEvent;
class QThread;

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

signals:
    void executionFinished(bool success);

private:
    void setupUi();

    void onPluginSelected(const std::string& pluginName);
    void onSubFunctionSelected(const std::string& pluginName, const std::string& actionKey);
    void onExecuteClicked();

    void onParamsChanged();
    void onWorkerFinished(bool success, const QString& message);
    void onProgressChanged(const QString& taskId, double percent);
    void onMessageLogged(const QString& taskId, const QString& msg);
    void onRerunTask(const QString& taskId);
    void onEditTask(const QString& taskId);

    void syncDerivedParams();
    void updateExecuteButtonState();

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

    QThread* workerThread_ = nullptr;
    ExecuteWorker* currentWorker_ = nullptr;
    QtProgressReporter* currentReporter_ = nullptr;
    ProgressDialog* progressDialog_ = nullptr;

    std::string currentPluginName_;
    std::string currentActionKey_;
    QString currentTaskId_;
};

}
