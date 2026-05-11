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

namespace gis_ai::gui {

class NavPanel;
class ParamWidget;

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

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    NavPanel* navPanel_ = nullptr;
    ParamWidget* paramWidget_ = nullptr;
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
};

}
