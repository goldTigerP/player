#pragma once

#include "media/FFmpegVideoStream.h"
#include <QTimer>
#include <QWidget>

class VideoWidget : public QWidget {
    Q_OBJECT

public:
    static VideoWidget *createVideoWidget(QWidget *parent = nullptr);

    explicit VideoWidget(QWidget *parent = nullptr) : QWidget(parent), m_playTimer(this) {
        m_playTimer.setTimerType(Qt::TimerType::PreciseTimer);
        connect(&m_playTimer, &QTimer::timeout, this, &VideoWidget::updateFrame);
    }

    void loadVideo(const QString &filePath);

protected:
    void mousePressEvent(QMouseEvent *event) override {
        m_isPlaying = !m_isPlaying;
        if (m_isPlaying) {
            m_playTimer.start();
        } else {
            m_playTimer.stop();
        }
        return QWidget::mousePressEvent(event);
    }

private:
    virtual void showPreview() = 0;
    virtual void updateFrame() = 0;

    // 媒体控制
    void play();
    void pause();
    void stop();
    void seekToTime(double seconds);

    // void mousePressEvent(QMouseEvent *event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;
    // void wheelEvent(QWheelEvent *event) override;

    // 播放控制
    QTimer m_playTimer;
    bool m_isPlaying{false};
    double m_currentTime;

    // 显示属性
    Qt::AspectRatioMode m_scaleMode;
    float m_zoomFactor;

protected:
    FFmpegVideoStream m_videoStream{};
    double m_duration;
};
