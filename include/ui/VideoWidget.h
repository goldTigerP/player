#pragma once

#include "media/FFmpegStream.h"
#include <QTimer>
#include <QWidget>

// 前向声明
class AudioPlayer;

class VideoWidget : public QWidget {
    Q_OBJECT

public:
    static VideoWidget *createVideoWidget(QWidget *parent = nullptr);

    explicit VideoWidget(QWidget *parent = nullptr)
        : QWidget(parent), m_playTimer(this), m_audioTimer(this) {
        // 设置视频定时器
        m_playTimer.setTimerType(Qt::TimerType::PreciseTimer);
        connect(&m_playTimer, &QTimer::timeout, this, &VideoWidget::updateFrame);

        // 设置音频定时器（更高频率处理音频帧）
        m_audioTimer.setTimerType(Qt::TimerType::PreciseTimer);
        connect(&m_audioTimer, &QTimer::timeout, this, &VideoWidget::updateAudio);
    }

    void loadVideo(const QString &filePath);

protected:
    void mousePressEvent(QMouseEvent *event) override {
        m_isPlaying = !m_isPlaying;
        if (m_isPlaying) {
            play();
        } else {
            pause();
        }
        return QWidget::mousePressEvent(event);
    }

private:
    virtual void showPreview() = 0;
    virtual void updateFrame() = 0;
    void updateAudio();

    // 媒体控制
    void play();
    void pause();
    void stop();
    void seekToTime(double seconds);

    // 播放控制
    QTimer m_playTimer;   // 视频帧定时器
    QTimer m_audioTimer;  // 音频帧定时器
    bool m_isPlaying{false};

    // 显示属性
    Qt::AspectRatioMode m_scaleMode{Qt::KeepAspectRatio};
    float m_zoomFactor{1.0f};

protected:
    FFmpegStream m_videoStream{};
    AudioPlayer *m_audioPlayer{nullptr};
    double m_currentTime;
    double m_duration;

private:
    void initializeAudioPlayer();
};
