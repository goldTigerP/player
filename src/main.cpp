#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTimer>

#include "ui/MainWindow.h"
#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QWidget>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

// 日志分类
Q_LOGGING_CATEGORY(multimedia, "multimedia")

// 初始化FFmpeg
void initializeFFmpeg() {
    // FFmpeg 4.0+不需要显式注册
    avformat_network_init();

    // 设置FFmpeg日志级别
    av_log_set_level(AV_LOG_WARNING);

    qCInfo(multimedia) << "FFmpeg initialized successfully";
    qCInfo(multimedia) << "libavcodec version:" << avcodec_version();
    qCInfo(multimedia) << "libavformat version:" << avformat_version();
}

// 创建应用程序目录
void setupApplicationDirectories() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir().mkpath(configDir);
    QDir().mkpath(cacheDir);
    QDir().mkpath(dataDir);

    qCInfo(multimedia) << "Config directory:" << configDir;
    qCInfo(multimedia) << "Cache directory:" << cacheDir;
    qCInfo(multimedia) << "Data directory:" << dataDir;
}

// 设置应用程序样式
void setupApplicationStyle(QApplication &app) {
    // 设置应用程序信息
    app.setApplicationName("MultimediaPlayer");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("PJH");
    app.setOrganizationDomain("PJH.com");

    // 设置样式
    app.setStyle(QStyleFactory::create("Fusion"));

    // 设置暗色主题
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    app.setPalette(darkPalette);

    // 设置样式表
    app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid "
                      "white; }"
                      "QSlider::groove:horizontal { border: 1px solid #999999; height: 8px; "
                      "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, "
                      "stop:1 #c4c4c4); margin: 2px 0; }"
                      "QSlider::handle:horizontal { background: qlineargradient(x1:0, y1:0, "
                      "x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f); border: 1px solid #5c5c5c; "
                      "width: 18px; margin: -2px 0; border-radius: 3px; }");
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    try {
        // 设置应用程序样式和信息
        setupApplicationStyle(app);

        // 创建应用程序目录
        setupApplicationDirectories();

        // 初始化FFmpeg
        initializeFFmpeg();

        // 创建并显示简单主窗口
        MainWindow window;
        window.show();

        // 不处理命令行参数了，简化逻辑
        qCInfo(multimedia) << "Application started successfully";

        return app.exec();

    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "Error",
                              QString("Application failed to start: %1").arg(e.what()));
        return -1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Error",
                              "Application failed to start due to unknown error.");
        return -1;
    }
}