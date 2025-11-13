#pragma once

#include <QAudio>
#include <QAudioFormat>
#include <QAudioSink>
#include <QBuffer>
#include <QIODevice>
#include <QMutex>
#include <QObject>
#include <QTimer>
#include <queue>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

class AudioBuffer : public QIODevice {
    Q_OBJECT

public:
    explicit AudioBuffer(QObject *parent = nullptr);

    void writeData(const QByteArray &data);
    void clear();
    bool isEmpty() const;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    std::queue<QByteArray> m_bufferQueue;
    mutable QMutex m_mutex;
    QByteArray m_currentBuffer;
    int m_currentPos;
};

class AudioPlayer : public QObject {
    Q_OBJECT

public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer();

    // 初始化音频播放器
    bool initialize(AVCodecContext *audioCodecContext);

    // 播放音频帧
    void playAudioFrame(AVFrame *frame);

    // 播放控制
    void start();
    void pause();
    void resume();
    void stop();
    void setVolume(qreal volume);

    // 状态查询
    double getCurrentTime() const { return m_currentTime; }
    bool isPlaying() const;
    qreal getVolume() const;

    // 缓冲区状态
    bool hasBufferedData() const;
    void clearBuffer();

signals:
    void positionChanged(qint64 position);
    void stateChanged(int state);  // Use int instead of QAudioSink::State for compatibility
    void bufferLevelChanged(int level);

private slots:
    void updatePosition();
    void onAudioStateChanged();  // Remove parameter for compatibility

private:
    // 私有方法
    bool setupAudioFormat(AVCodecContext *codecContext);
    QByteArray convertAudioFrame(AVFrame *frame);
    void updateTimeFromFrame(AVFrame *frame);

private:
    QAudioFormat m_audioFormat;
    QAudioSink *m_audioSink;
    AudioBuffer *m_audioBuffer;
    SwrContext *m_swrContext;
    QTimer *m_positionTimer;

    double m_currentTime;
    int m_sampleRate;
    int m_channels;
    AVRational m_timeBase;
    bool m_initialized;
};
