#pragma once

#include <QtCore/QFileInfo>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 公共方法
    void openFile(const QString &filePath);

  protected:
    void closeEvent(QCloseEvent *event) override;

  private slots:
    void openFile();
    void showAbout();

  private:
    // 私有方法
    void setupUI();
    void setupMenus();

    // UI组件
    QTabWidget *m_centralWidget;

    // 状态
    bool m_isFullscreen;
    bool m_playlistVisible;
};