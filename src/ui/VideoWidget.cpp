#include "ui/VideoWidget.h"
#include "AudioPlayer.h"
#include "OpenGLVideoWidget.h"
#include "media/FFmpegStream.h"
#include <QDebug>

VideoWidget *VideoWidget::createVideoWidget(QWidget *parent) {
    return new OpenGLVideoWidget(parent);
}

void VideoWidget::loadVideo(const QString &filePath) {
    m_videoStream.loadVideo(filePath);
    auto fps = m_videoStream.getFps();
    if (fps > 0) {
        m_playTimer.setInterval(1000 / fps);
    }

    // 初始化音频播放器
    initializeAudioPlayer();

    showPreview();
}

void VideoWidget::initializeAudioPlayer() {
    // 如果已经有音频播放器，先清理
    if (m_audioPlayer) {
        m_audioPlayer->stop();
        m_audioPlayer->deleteLater();
        m_audioPlayer = nullptr;
    }

    // 检查是否有音频流
    if (!m_videoStream.hasAudio()) {
        qDebug() << "视频文件不包含音频流";
        return;
    }

    // 获取音频编解码器上下文
    AVCodecContext *audioCodecContext = m_videoStream.getAudioCodecContext();
    if (!audioCodecContext) {
        qDebug() << "无法获取音频编解码器上下文";
        return;
    }

    // 创建并初始化音频播放器
    m_audioPlayer = new AudioPlayer(this);
    if (m_audioPlayer->initialize(audioCodecContext)) {
        qDebug() << "音频播放器初始化完成";
    } else {
        qDebug() << "音频播放器初始化失败";
        m_audioPlayer->deleteLater();
        m_audioPlayer = nullptr;
    }
}

void VideoWidget::play() {
    // 启动视频定时器 (通常25-30fps)
    m_playTimer.start(33);  // ~30fps

    // 启动音频定时器 (更高频率处理音频帧)
    m_audioTimer.start(10);  // 100Hz，确保音频连续

    m_videoStream.play();

    if (m_audioPlayer) {
        m_audioPlayer->start();
        qDebug() << "开始播放音频";
    }
}

void VideoWidget::pause() {
    m_playTimer.stop();
    m_audioTimer.stop();  // 同时停止音频定时器
    m_videoStream.pause();

    if (m_audioPlayer) {
        m_audioPlayer->pause();
        qDebug() << "暂停音频播放";
    }
}

void VideoWidget::stop() {
    m_playTimer.stop();
    m_audioTimer.stop();  // 同时停止音频定时器
    m_isPlaying = false;
    m_videoStream.stop();

    if (m_audioPlayer) {
        m_audioPlayer->stop();
        qDebug() << "停止音频播放";
    }
}

void VideoWidget::seekToTime(double seconds) {
    m_videoStream.seek(seconds);
    m_currentTime = seconds;

    if (m_audioPlayer) {
        // 音频播放器需要清空缓冲区重新开始
        m_audioPlayer->clearBuffer();
    }
}

void VideoWidget::updateAudio() {
    // 专门处理音频帧
    if (m_audioPlayer && m_isPlaying) {
        double audioPts = 0.0;
        AVFrame *audioFrame = m_videoStream.getNextAudioFrame(&audioPts);
        if (audioFrame) {
            m_audioPlayer->playAudioFrame(audioFrame);
            av_frame_free(&audioFrame);
        }
    }
}
