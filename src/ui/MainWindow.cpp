#include "ui/MainWindow.h"
#include <QApplication>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include "core/FFmpegMediaDetector.h"


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_isFullscreen(false), m_playlistVisible(true) {
    setupUI();
    setWindowTitle("Multimedia Player");
    resize(1200, 800);
}

MainWindow::~MainWindow() {
    // 析构函数
}

void MainWindow::setupUI() {
    // 创建中央部件
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // 创建简单的布局
    QVBoxLayout* layout = new QVBoxLayout(m_centralWidget);
    QLabel* placeholder = new QLabel("多媒体播放器 - 准备就绪", this);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("font-size: 24px; color: #666; padding: 50px;");

    layout->addWidget(placeholder);

    // 创建菜单栏
    setupMenus();

    // 创建状态栏
    statusBar()->showMessage("就绪");
}

void MainWindow::setupMenus() {
    m_fileMenu = menuBar()->addMenu("文件(&F)");

    QAction* openAction = m_fileMenu->addAction("打开文件(&O)");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, [this]() { openFile(); });

    m_fileMenu->addSeparator();

    QAction* exitAction = m_fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    m_helpMenu = menuBar()->addMenu("帮助(&H)");
    QAction* aboutAction = m_helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, this, [this]() { showAbout(); });
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(
        this, "打开媒体文件", "",
        "所有支持的文件 (*.mp4 *.avi *.mkv *.mov *.mp3 *.wav *.jpg *.png *.bmp);;所有文件 (*.*)");

    if (!fileName.isEmpty()) {
        statusBar()->showMessage(QString("打开文件: %1").arg(QFileInfo(fileName).baseName()));
        FFmpegMediaDetector::detectMediaType(fileName);
    }
}

void MainWindow::openFile(const QString& filePath) {
    if (!filePath.isEmpty()) {
        statusBar()->showMessage(QString("打开文件: %1").arg(QFileInfo(filePath).baseName()));
        // TODO: 实际的文件打开逻辑
    }
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "关于",
                       "多媒体播放器 v1.0\n\n"
                       "基于 Qt, FFmpeg 和 OpenCV 构建的跨平台多媒体播放器\n\n"
                       "支持视频播放、音频播放和图片查看功能");
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // 保存设置等清理工作
    QMainWindow::closeEvent(event);
}