#include "ui/MainWindow.h"

#include "media/FFmpegMediaDetector.h"
#include "ui/ImageWidget.h"
#include "ui/VideoWidget.h"
#include <QApplication>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
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
    m_centralWidget = new QTabWidget(this);
    m_centralWidget->setTabsClosable(true);
    connect(m_centralWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    setCentralWidget(m_centralWidget);

    // 创建菜单栏
    setupMenus();

    // 创建状态栏
    statusBar()->showMessage("就绪");
}

void MainWindow::setupMenus() {
    auto fileMenu = menuBar()->addMenu("文件(&F)");

    QAction *openAction = fileMenu->addAction("打开文件(&O)");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, [this]() { openFile(); });

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto helpMenu = menuBar()->addMenu("帮助(&H)");
    QAction *aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, this, [this]() { showAbout(); });
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(
        this, "打开媒体文件", "",
        "所有支持的文件 (*.mp4 *.avi *.mkv *.mov *.mp3 *.wav *.jpg *.png *.bmp);;所有文件 (*.*)");

    if (!fileName.isEmpty()) {
        auto fileBaseName = QFileInfo(fileName).baseName();
        auto fileType = FFmpegMediaDetector::detectMediaType(fileName);
        if (fileType == MediaType::Image) {
            auto widget = ImageWidget::createImageWidget(nullptr);
            widget->loadImage(fileName);
            auto index = m_centralWidget->addTab((QWidget *)widget, fileBaseName);
            m_centralWidget->setCurrentIndex(index);
        } else if (fileType == MediaType::Video) {
            auto widget = VideoWidget::createVideoWidget(nullptr);
            widget->loadVideo(fileName);
            auto index = m_centralWidget->addTab((QWidget *)widget, fileBaseName);
            m_centralWidget->setCurrentIndex(index);
        }
        statusBar()->showMessage(QString("打开文件: %1").arg(fileBaseName));
    }
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "关于",
                       "多媒体播放器 v1.0\n\n"
                       "基于 Qt, FFmpeg 和 OpenCV 构建的跨平台多媒体播放器\n\n"
                       "支持视频播放、音频播放和图片查看功能");
}

void MainWindow::closeTab(int index) {
    if (index < 0 || index >= m_centralWidget->count()) {
        qDebug() << "无效的tab索引:" << index;
        return;
    }

    QWidget *widget = m_centralWidget->widget(index);
    QString tabText = m_centralWidget->tabText(index);

    // // 询问用户是否确认关闭
    // QMessageBox::StandardButton reply =
    //     QMessageBox::question(this, "确认关闭", QString("确定要关闭 '%1' 吗？").arg(tabText),
    //                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    // if (reply != QMessageBox::Yes) {
    //     return;
    // }

    m_centralWidget->removeTab(index);
    widget->deleteLater();

    // 更新状态栏
    statusBar()->showMessage(QString("已关闭: %1").arg(tabText), 2000);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // 保存设置等清理工作
    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    if (m_centralWidget != nullptr) {
        m_centralWidget->resize(event->size().width(), event->size().height());
    }
}