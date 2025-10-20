#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>
#include <QtCore/QTimer>
#include <memory>

#include "MediaManager.h"
#include "MediaDisplayWidget.h"
#include "PlaylistWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    // 文件操作
    void openFile();
    void openDirectory();
    void openRecent();
    
    // 播放控制
    void play();
    void pause();
    void stop();
    void previous();
    void next();
    void seek();
    void setVolume(int volume);
    void toggleMute();
    
    // 视图控制
    void toggleFullscreen();
    void togglePlaylist();
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void actualSize();
    
    // 媒体管理器信号
    void onFileOpened(const QString& filePath);
    void onMediaTypeChanged(MediaFileType type);
    void onPositionChanged(int64_t position);
    void onDurationChanged(int64_t duration);
    void onStateChanged(PlayState state);
    
    // 界面更新
    void updateControls();
    void updateTimeDisplay();
    void updateWindowTitle();
    void updateStatusBar();
    
    // 设置
    void showPreferences();
    void showAbout();

private:
    // 核心组件
    std::unique_ptr<MediaManager> m_mediaManager;
    
    // 主要UI组件
    QWidget* m_centralWidget;
    QSplitter* m_mainSplitter;
    MediaDisplayWidget* m_displayWidget;
    PlaylistWidget* m_playlistWidget;
    
    // 控制面板
    QWidget* m_controlPanel;
    QPushButton* m_playButton;
    QPushButton* m_stopButton;
    QPushButton* m_previousButton;
    QPushButton* m_nextButton;
    QSlider* m_positionSlider;
    QSlider* m_volumeSlider;
    QPushButton* m_muteButton;
    QLabel* m_timeLabel;
    QLabel* m_durationLabel;
    QComboBox* m_playModeCombo;
    
    // 菜单和工具栏
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QStatusBar* m_statusBar;
    
    // 菜单
    QMenu* m_fileMenu;
    QMenu* m_playMenu;
    QMenu* m_viewMenu;
    QMenu* m_toolsMenu;
    QMenu* m_helpMenu;
    
    // 动作
    QAction* m_openFileAction;
    QAction* m_openDirAction;
    QAction* m_exitAction;
    QAction* m_playAction;
    QAction* m_pauseAction;
    QAction* m_stopAction;
    QAction* m_previousAction;
    QAction* m_nextAction;
    QAction* m_fullscreenAction;
    QAction* m_playlistAction;
    QAction* m_preferencesAction;
    QAction* m_aboutAction;
    
    // 状态
    bool m_isFullscreen;
    bool m_playlistVisible;
    QString m_currentFilePath;
    
    // 定时器
    QTimer* m_updateTimer;
    
    // 私有方法
    void setupUI();
    void setupMenus();
    void setupToolBar();
    void setupStatusBar();
    void setupActions();
    void setupControlPanel();
    void setupLayout();
    void connectSignals();
    
    void loadSettings();
    void saveSettings();
    
    QString formatTime(int64_t milliseconds) const;
    void setControlsEnabled(bool enabled);
    void updatePlayButton();
    void addRecentFile(const QString& filePath);
};