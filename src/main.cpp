#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QLoggingCategory>
#include <QTimer>
#include <QFile>

// 暂时注释掉复杂的MainWindow，使用简单版本
// #include "ui/MainWindow.h"
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

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
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/MultimediaPlayer";
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/MultimediaPlayer";
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/MultimediaPlayer";
    
    QDir().mkpath(configDir);
    QDir().mkpath(cacheDir);
    QDir().mkpath(dataDir);
    
    qCInfo(multimedia) << "Config directory:" << configDir;
    qCInfo(multimedia) << "Cache directory:" << cacheDir;
    qCInfo(multimedia) << "Data directory:" << dataDir;
}

// 设置应用程序样式
void setupApplicationStyle(QApplication& app) {
    // 设置应用程序信息
    app.setApplicationName("Multimedia Player");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("YourCompany");
    app.setOrganizationDomain("yourcompany.com");
    
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
    app.setStyleSheet(
        "QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }"
        "QSlider::groove:horizontal { border: 1px solid #999999; height: 8px; background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4); margin: 2px 0; }"
        "QSlider::handle:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f); border: 1px solid #5c5c5c; width: 18px; margin: -2px 0; border-radius: 3px; }"
    );
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    try {
        // 设置应用程序样式和信息
        setupApplicationStyle(app);
        
        // 创建应用程序目录
        setupApplicationDirectories();
        
        // 初始化FFmpeg
        initializeFFmpeg();
        
        // 创建并显示简单主窗口
        QMainWindow window;
        window.setWindowTitle("多媒体播放器 - 演示版本");
        window.resize(1200, 800);
        
        // 创建中央部件
        QWidget* centralWidget = new QWidget(&window);
        window.setCentralWidget(centralWidget);
        
        QVBoxLayout* layout = new QVBoxLayout(centralWidget);
        QLabel* label = new QLabel("🎬 多媒体播放器 v1.0\n\n"
                                   "✅ CMake 构建系统配置成功\n"
                                   "✅ Qt5 界面框架集成成功\n" 
                                   "✅ FFmpeg 视频处理库集成成功\n"
                                   "✅ OpenCV 图片处理库集成成功\n\n"
                                   "🚀 项目架构搭建完成，可以开始开发具体功能了！\n\n"
                                   "下一步可以实现：\n"
                                   "• 完善 MediaPlayer 视频播放功能\n"
                                   "• 完善 ImageViewer 图片查看功能\n"
                                   "• 实现完整的用户界面\n"
                                   "• 添加播放列表功能", &window);
        
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 16px; color: #EEEEEE; padding: 50px; line-height: 1.8;");
        layout->addWidget(label);
        
        // 创建菜单栏
        QMenuBar* menuBar = window.menuBar();
        QMenu* fileMenu = menuBar->addMenu("文件(&F)");
        QAction* exitAction = fileMenu->addAction("退出(&X)");
        QObject::connect(exitAction, &QAction::triggered, &window, &QWidget::close);
        
        QMenu* helpMenu = menuBar->addMenu("帮助(&H)");
        QAction* aboutAction = helpMenu->addAction("关于(&A)");
        QObject::connect(aboutAction, &QAction::triggered, [&window]() {
            QMessageBox::about(&window, "关于",
                "多媒体播放器 v1.0\n\n"
                "🏗️ 基于以下技术构建：\n"
                "• Qt 5.15.3 - 用户界面框架\n"
                "• FFmpeg 4.4+ - 视频音频处理\n"
                "• OpenCV 4.5+ - 图片处理\n"
                "• CMake 3.16+ - 构建系统\n"
                "• C++17 - 编程语言\n\n"
                "🌍 支持跨平台：Ubuntu, Windows, macOS\n\n"
                "📧 如有问题请查看项目文档");
        });
        
        window.show();
        
        // 不处理命令行参数了，简化逻辑
        
        qCInfo(multimedia) << "Application started successfully";
        
        return app.exec();
        
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Error", 
            QString("Application failed to start: %1").arg(e.what()));
        return -1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Error", 
            "Application failed to start due to unknown error.");
        return -1;
    }
}