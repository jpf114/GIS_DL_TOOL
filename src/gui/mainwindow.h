#pragma once

#include <QMainWindow>
#include <QString>

class QFrame;
class QLabel;
class QPushButton;
class QTabWidget;
class QProgressBar;
class QDragEnterEvent;
class QDropEvent;

namespace gis_ai::gui {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

signals:
    void executionFinished(bool success);

private:
    void setupUi();

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    QFrame* sidebar_ = nullptr;
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
