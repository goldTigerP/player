#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFileInfo>
#include <QtGui/QCloseEvent>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // 公共方法
    void openFile(const QString& filePath);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void openFile();
    void showAbout();

private:
    // UI组件
    QWidget* m_centralWidget;
    QMenuBar* m_menuBar;
    QStatusBar* m_statusBar;
    
    // 菜单
    QMenu* m_fileMenu;
    QMenu* m_helpMenu;
    
    // 状态
    bool m_isFullscreen;
    bool m_playlistVisible;
    
    // 私有方法
    void setupUI();
    void setupMenus();
};