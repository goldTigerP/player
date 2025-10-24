#pragma once

#include "media/FFmpegVideoStream.h"
#include <QWidget>

class VideoWidget : public QWidget {
    Q_OBJECT

public:
    static VideoWidget *createVideoWidget(QWidget *parent = nullptr);

    explicit VideoWidget(QWidget *parent = nullptr) : QWidget(parent) {}

    void loadVideo(const QString &filePath);

private slots:
    // void updateFrame();

private:
    virtual void showPreview() = 0;

    // 媒体控制
    void play();
    void pause();
    void stop();
    void seekToTime(double seconds);

    // 显示控制
    void setScaleMode(Qt::AspectRatioMode mode);
    void setZoom(float factor);

    // void mousePressEvent(QMouseEvent *event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;
    // void wheelEvent(QWheelEvent *event) override;

    // 播放控制
    QTimer *m_playTimer;
    bool m_isPlaying;
    double m_currentTime;
    double m_duration;

    // 显示属性
    Qt::AspectRatioMode m_scaleMode;
    float m_zoomFactor;

protected:
    FFmpegVideoStream m_videoStream{};
};
