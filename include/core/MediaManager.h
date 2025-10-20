#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <memory>

#include "MediaPlayer.h"
#include "ImageViewer.h"

enum class MediaFileType {
    Unknown,
    Video,
    Audio,
    Image
};

struct MediaFile {
    QString filePath;
    QString fileName;
    MediaFileType type;
    qint64 size;
    QDateTime lastModified;
};

class MediaManager : public QObject {
    Q_OBJECT

public:
    explicit MediaManager(QObject* parent = nullptr);
    ~MediaManager();

    // 文件管理
    bool openFile(const QString& filePath);
    bool openDirectory(const QString& dirPath);
    void clearPlaylist();
    
    // 播放列表管理
    QStringList getPlaylist() const { return m_playlist; }
    int getCurrentIndex() const { return m_currentIndex; }
    bool setCurrentIndex(int index);
    
    // 导航
    bool hasNext() const;
    bool hasPrevious() const;
    bool next();
    bool previous();
    
    // 文件信息
    MediaFile getCurrentFile() const;
    QList<MediaFile> getAllFiles() const { return m_mediaFiles; }
    
    // 媒体播放器访问
    std::shared_ptr<MediaPlayer> getMediaPlayer() const { return m_mediaPlayer; }
    std::shared_ptr<ImageViewer> getImageViewer() const { return m_imageViewer; }
    
    // 文件过滤
    void setVideoExtensions(const QStringList& extensions) { m_videoExtensions = extensions; }
    void setAudioExtensions(const QStringList& extensions) { m_audioExtensions = extensions; }
    void setImageExtensions(const QStringList& extensions) { m_imageExtensions = extensions; }
    
    QStringList getVideoExtensions() const { return m_videoExtensions; }
    QStringList getAudioExtensions() const { return m_audioExtensions; }
    QStringList getImageExtensions() const { return m_imageExtensions; }
    
    // 搜索和过滤
    void filterByType(MediaFileType type);
    void searchFiles(const QString& keyword);
    void resetFilter();
    
    // 播放模式
    enum PlayMode {
        Sequential,
        Loop,
        Shuffle,
        RepeatOne
    };
    
    void setPlayMode(PlayMode mode) { m_playMode = mode; }
    PlayMode getPlayMode() const { return m_playMode; }
    
    // 静态方法
    static MediaFileType getFileType(const QString& filePath);
    static QStringList getDefaultVideoExtensions();
    static QStringList getDefaultAudioExtensions();
    static QStringList getDefaultImageExtensions();
    static bool isMediaFile(const QString& filePath);

signals:
    void fileOpened(const QString& filePath);
    void playlistChanged();
    void currentIndexChanged(int index);
    void mediaTypeChanged(MediaFileType type);
    void error(const QString& errorString);

private slots:
    void onMediaPlayerStateChanged(PlayState state);
    void onMediaPlayerFinished();

private:
    // 媒体处理器
    std::shared_ptr<MediaPlayer> m_mediaPlayer;
    std::shared_ptr<ImageViewer> m_imageViewer;
    
    // 播放列表
    QStringList m_playlist;
    QList<MediaFile> m_mediaFiles;
    int m_currentIndex;
    
    // 文件扩展名
    QStringList m_videoExtensions;
    QStringList m_audioExtensions;
    QStringList m_imageExtensions;
    
    // 播放模式
    PlayMode m_playMode;
    
    // 过滤状态
    bool m_isFiltered;
    QList<MediaFile> m_filteredFiles;
    QStringList m_filteredPlaylist;
    
    // 私有方法
    void scanDirectory(const QString& dirPath);
    void addMediaFile(const QString& filePath);
    void updatePlaylist();
    MediaFile createMediaFile(const QString& filePath);
    int getNextIndex() const;
    int getPreviousIndex() const;
    void shufflePlaylist();
};