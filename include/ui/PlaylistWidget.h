#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QContextMenuEvent>
#include <QtCore/QFileInfo>
#include <memory>

#include "MediaManager.h"

class PlaylistItem : public QListWidgetItem {
public:
    explicit PlaylistItem(const MediaFile& mediaFile, QListWidget* parent = nullptr);
    
    MediaFile getMediaFile() const { return m_mediaFile; }
    void updateDisplay();
    
private:
    MediaFile m_mediaFile;
    void setupDisplay();
};

class PlaylistWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlaylistWidget(QWidget* parent = nullptr);
    ~PlaylistWidget();

    // 媒体管理器
    void setMediaManager(std::shared_ptr<MediaManager> manager);
    
    // 播放列表控制
    void updatePlaylist();
    void clearPlaylist();
    void selectCurrentItem();
    
    // 搜索和过滤
    void setSearchText(const QString& text);
    void setFilterType(MediaFileType type);
    void clearFilter();

signals:
    void itemActivated(int index);
    void itemRemoved(int index);
    void playlistCleared();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void onItemDoubleClicked(QListWidgetItem* item);
    void onItemSelectionChanged();
    void onSearchTextChanged();
    void onFilterChanged();
    void onCurrentIndexChanged(int index);
    void onPlaylistChanged();
    
    // 上下文菜单动作
    void playSelected();
    void removeSelected();
    void removeAll();
    void showInExplorer();
    void showProperties();

private:
    // UI组件
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_headerLayout;
    QHBoxLayout* m_searchLayout;
    
    QLabel* m_titleLabel;
    QPushButton* m_clearButton;
    
    QLineEdit* m_searchEdit;
    QComboBox* m_filterCombo;
    
    QListWidget* m_playlistView;
    
    QLabel* m_statusLabel;
    
    // 媒体管理器
    std::shared_ptr<MediaManager> m_mediaManager;
    
    // 上下文菜单
    QMenu* m_contextMenu;
    QAction* m_playAction;
    QAction* m_removeAction;
    QAction* m_removeAllAction;
    QAction* m_showInExplorerAction;
    QAction* m_propertiesAction;
    
    // 状态
    QString m_searchText;
    MediaFileType m_filterType;
    
    // 私有方法
    void setupUI();
    void setupContextMenu();
    void connectSignals();
    void updateStatusLabel();
    PlaylistItem* getPlaylistItem(int index) const;
    int getItemIndex(PlaylistItem* item) const;
    void addMediaFile(const MediaFile& mediaFile);
    QString getTypeString(MediaFileType type) const;
    QString formatFileSize(qint64 bytes) const;
    QString formatDuration(int64_t milliseconds) const;
};