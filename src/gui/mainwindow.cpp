#include "mainwindow.h"
#include "style_constants.h"

#include <QApplication>
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

}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setAcceptDrops(true);
    setupUi();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("GIS AI 工具台"));
    resize(Size::kWindowDefaultWidth, Size::kWindowDefaultHeight);
    setMinimumSize(Size::kWindowMinWidth, Size::kWindowMinHeight);
    setStyleSheet(globalStyleSheet());

    auto* centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    sidebar_ = new QFrame;
    sidebar_->setObjectName(QStringLiteral("sidebar"));
    sidebar_->setStyleSheet(sidebarStyleSheet());
    sidebar_->setFixedWidth(Size::kSidebarWidth);

    auto* sidebarLayout = new QVBoxLayout(sidebar_);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    auto* sidebarTopCard = new QFrame;
    sidebarTopCard->setObjectName(QStringLiteral("sidebarTopCard"));
    auto* topLayout = new QVBoxLayout(sidebarTopCard);
    topLayout->setContentsMargins(20, 24, 20, 16);
    topLayout->setSpacing(6);

    auto* eyebrowLabel = new QLabel(QStringLiteral("GIS AI TOOLKIT"));
    eyebrowLabel->setObjectName(QStringLiteral("sidebarEyebrow"));
    topLayout->addWidget(eyebrowLabel);

    auto* titleLabel = new QLabel(QStringLiteral("GIS AI 工具台"));
    titleLabel->setObjectName(QStringLiteral("sidebarTitle"));
    topLayout->addWidget(titleLabel);

    auto* descLabel = new QLabel(QStringLiteral("地理空间 AI 算法工作台"));
    descLabel->setObjectName(QStringLiteral("sidebarDesc"));
    descLabel->setWordWrap(true);
    topLayout->addWidget(descLabel);

    sidebarLayout->addWidget(sidebarTopCard);

    auto* sidebarDivider = new QFrame;
    sidebarDivider->setObjectName(QStringLiteral("sidebarDivider"));
    sidebarLayout->addWidget(sidebarDivider);

    auto* sidebarScrollArea = new QScrollArea;
    sidebarScrollArea->setWidgetResizable(true);
    sidebarScrollArea->setFrameShape(QFrame::NoFrame);
    sidebarScrollArea->setStyleSheet(
        QStringLiteral("QScrollArea { background: transparent; border: none; }"));

    auto* navContent = new QWidget;
    navContent->setStyleSheet(QStringLiteral("background: transparent;"));
    auto* navLayout = new QVBoxLayout(navContent);
    navLayout->setContentsMargins(8, 12, 8, 12);
    navLayout->setSpacing(2);

    auto* sectionLabel = new QLabel(QStringLiteral("主功能"));
    sectionLabel->setObjectName(QStringLiteral("sidebarSection"));
    navLayout->addWidget(sectionLabel);

    navLayout->addStretch();
    sidebarScrollArea->setWidget(navContent);
    sidebarLayout->addWidget(sidebarScrollArea, 1);

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

    auto* paramPlaceholder = new QFrame;
    paramPlaceholder->setObjectName(QStringLiteral("card"));
    auto* paramPlaceholderLayout = new QVBoxLayout(paramPlaceholder);
    paramPlaceholderLayout->setContentsMargins(Size::kCardPadding, Size::kCardPadding, Size::kCardPadding, Size::kCardPadding);
    auto* paramPlaceholderLabel = new QLabel(QStringLiteral("参数面板占位区域"));
    paramPlaceholderLabel->setObjectName(QStringLiteral("cardDesc"));
    paramPlaceholderLabel->setAlignment(Qt::AlignCenter);
    paramPlaceholderLayout->addWidget(paramPlaceholderLabel);
    paramScrollArea->setWidget(paramPlaceholder);

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

    auto* taskCenterPlaceholder = new QFrame;
    taskCenterPlaceholder->setObjectName(QStringLiteral("card"));
    auto* taskPlaceholderLayout = new QVBoxLayout(taskCenterPlaceholder);
    taskPlaceholderLayout->setContentsMargins(Size::kCardPadding, Size::kCardPadding, Size::kCardPadding, Size::kCardPadding);
    auto* taskPlaceholderLabel = new QLabel(QStringLiteral("任务中心占位区域"));
    taskPlaceholderLabel->setObjectName(QStringLiteral("cardDesc"));
    taskPlaceholderLabel->setAlignment(Qt::AlignCenter);
    taskPlaceholderLayout->addWidget(taskPlaceholderLabel);
    tabWidget_->addTab(taskCenterPlaceholder, QStringLiteral("任务中心"));

    mainLayout->addWidget(sidebar_);
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

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    if (!event->mimeData()->hasUrls()) return;
    event->acceptProposedAction();
}

}
