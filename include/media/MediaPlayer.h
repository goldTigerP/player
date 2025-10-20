#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

enum class PlayState {
    Stopped,
    Playing,
    Paused
};

enum class MediaType {
    Unknown,
    Video,
    Audio,
    Image
};

struct MediaInfo {
    QString filePath;
    MediaType type;
    int64_t duration;  // 持续时间（毫秒）
    int width;
    int height;
    int fps;
    QString codec;
    int64_t bitrate;
};

class MediaPlayer : public QObject {
    Q_OBJECT

public:
    explicit MediaPlayer(QObject* parent = nullptr);
    ~MediaPlayer();

    // 基本播放控制
    bool openMedia(const QString& filePath);
    void play();
    void pause();
    void stop();
    void seek(int64_t position); // 毫秒

    // 状态查询
    PlayState getState() const { return m_state; }
    int64_t getCurrentPosition() const { return m_currentPosition; }
    int64_t getDuration() const { return m_mediaInfo.duration; }
    MediaInfo getMediaInfo() const { return m_mediaInfo; }
    
    // 音量控制
    void setVolume(int volume); // 0-100
    int getVolume() const { return m_volume; }
    void setMuted(bool muted);
    bool isMuted() const { return m_muted; }

    // 播放速度控制
    void setPlaybackRate(double rate);
    double getPlaybackRate() const { return m_playbackRate; }

signals:
    void stateChanged(PlayState state);
    void positionChanged(int64_t position);
    void durationChanged(int64_t duration);
    void frameReady(const QByteArray& frameData, int width, int height);
    void audioDataReady(const QByteArray& audioData);
    void error(const QString& errorString);
    void mediaInfoChanged(const MediaInfo& info);

private slots:
    void updatePosition();

private:
    // FFmpeg相关
    AVFormatContext* m_formatContext;
    AVCodecContext* m_videoCodecContext;
    AVCodecContext* m_audioCodecContext;
    SwsContext* m_swsContext;
    SwrContext* m_swrContext;
    
    int m_videoStreamIndex;
    int m_audioStreamIndex;
    
    // 播放状态
    PlayState m_state;
    MediaInfo m_mediaInfo;
    int64_t m_currentPosition; // 毫秒
    int m_volume;
    bool m_muted;
    double m_playbackRate;
    
    // 定时器
    QTimer* m_positionTimer;
    
    // 私有方法
    bool initializeFFmpeg();
    void cleanup();
    bool openVideoCodec(AVStream* stream);
    bool openAudioCodec(AVStream* stream);
    MediaType detectMediaType(const QString& filePath);
    void extractMediaInfo();
    QByteArray convertVideoFrame(AVFrame* frame);
    QByteArray convertAudioFrame(AVFrame* frame);
};