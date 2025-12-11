#pragma once

#include <QMutex>
#include <QQueue>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>
#include <atomic>
#include <deque>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
}

// 前向声明内部类
class DemuxThread;
class VideoDecoder;
class AudioDecoder;
class FrameCache;

// 帧数据结构
struct FrameData {
    AVFrame *frame{nullptr};
    double pts{0.0};  // 时间戳
    int64_t duration{0};

    FrameData() = default;
    FrameData(AVFrame *f, double p = 0.0) : frame(f), pts(p) {}
    ~FrameData() {
        if (frame) {
            av_frame_free(&frame);
        }
    }

    // 移动构造函数
    FrameData(FrameData &&other) noexcept {
        frame = other.frame;
        pts = other.pts;
        duration = other.duration;
        other.frame = nullptr;
    }

    FrameData &operator=(FrameData &&other) noexcept {
        if (this != &other) {
            if (frame) av_frame_free(&frame);
            frame = other.frame;
            pts = other.pts;
            duration = other.duration;
            other.frame = nullptr;
        }
        return *this;
    }

    // 禁用拷贝
    FrameData(const FrameData &) = delete;
    FrameData &operator=(const FrameData &) = delete;
};

// 数据包结构
struct PacketData {
    AVPacket *packet{nullptr};
    double pts{0.0};

    PacketData() = default;
    PacketData(AVPacket *p, double time = 0.0) : packet(p), pts(time) {}
    ~PacketData() {
        if (packet) {
            av_packet_free(&packet);
        }
    }

    // 移动构造
    PacketData(PacketData &&other) noexcept {
        packet = other.packet;
        pts = other.pts;
        other.packet = nullptr;
    }

    PacketData &operator=(PacketData &&other) noexcept {
        if (this != &other) {
            if (packet) av_packet_free(&packet);
            packet = other.packet;
            pts = other.pts;
            other.packet = nullptr;
        }
        return *this;
    }

    // 禁用拷贝
    PacketData(const PacketData &) = delete;
    PacketData &operator=(const PacketData &) = delete;
};

class FFmpegStream : public QObject {
    Q_OBJECT

public:
    explicit FFmpegStream(QObject *parent = nullptr);
    ~FFmpegStream();

    bool loadVideo(const QString &filePath);

    AVFrame *getNextVideoFrame(double *pts = nullptr);

    AVFrame *getNextAudioFrame(double *pts = nullptr);

    AVFrame *getPreviewImage();

    double getFps() const { return m_fps; }
    double getDuration() const { return m_duration; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    bool hasVideo() const { return m_hasVideo; }
    bool hasAudio() const { return m_hasAudio; }

    AVCodecContext *getAudioCodecContext() const;

    void play();
    void pause();
    void stop();
    void seek(double seconds);
    bool isPlaying() const { return m_isPlaying; }

    void setMaxVideoFrames(int maxFrames) { m_maxVideoFrames = maxFrames; }
    void setMaxAudioFrames(int maxFrames) { m_maxAudioFrames = maxFrames; }
    int getVideoFramesInCache() const;
    int getAudioFramesInCache() const;

signals:
    void loadFinished(bool success);
    void endOfStream();
    void errorOccurred(const QString &error);

private slots:
    void onDemuxFinished();
    void onVideoDecodeError();
    void onAudioDecodeError();

private:
    // ============== 内部状态 ==============
    QString m_filePath;
    double m_fps{0.0};
    double m_duration{0.0};
    int m_width{0};
    int m_height{0};
    bool m_hasVideo{false};
    bool m_hasAudio{false};
    std::atomic<bool> m_isPlaying{false};
    std::atomic<bool> m_isLoaded{false};

    // FFmpeg上下文
    AVFormatContext *m_formatContext{nullptr};
    int m_videoStreamIndex{-1};
    int m_audioStreamIndex{-1};
    AVCodecContext *m_audioCodecContext{nullptr};

    // 缓存控制
    int m_maxVideoFrames{30};   // 最多缓存30个视频帧
    int m_maxAudioFrames{100};  // 最多缓存100个音频帧

    // ============== 内部工作线程 ==============
    std::unique_ptr<DemuxThread> m_demuxThread;
    std::unique_ptr<VideoDecoder> m_videoDecoder;
    std::unique_ptr<AudioDecoder> m_audioDecoder;
    std::unique_ptr<FrameCache> m_frameCache;

    // ============== 内部方法 ==============
    void cleanup();
    bool initializeStreams();
    void startThreads();
    void stopThreads();
};

class DemuxThread : public QThread {
    Q_OBJECT

public:
    explicit DemuxThread(FFmpegStream *parent);
    ~DemuxThread();

    void setFormatContext(AVFormatContext *ctx, int videoIndex, int audioIndex);
    void requestStop();
    void seek(double seconds);

    // 队列访问接口
    bool getVideoPacket(std::unique_ptr<PacketData> &packet);
    bool getAudioPacket(std::unique_ptr<PacketData> &packet);

    bool isVideoQueueFull() const;
    bool isAudioQueueFull() const;

protected:
    void run() override;

signals:
    void finished();
    void errorOccurred(const QString &error);

private:
    FFmpegStream *m_parent;
    AVFormatContext *m_formatContext{nullptr};
    int m_videoStreamIndex{-1};
    int m_audioStreamIndex{-1};

    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_seekRequested{false};
    std::atomic<double> m_seekTime{0.0};

    // 数据包队列
    std::deque<std::unique_ptr<PacketData>> m_videoPacketQueue;
    std::deque<std::unique_ptr<PacketData>> m_audioPacketQueue;

    // 队列保护
    mutable QMutex m_videoMutex;
    mutable QMutex m_audioMutex;
    QWaitCondition m_videoCondition;
    QWaitCondition m_audioCondition;

    // 队列限制
    static const int MAX_VIDEO_PACKETS = 50;
    static const int MAX_AUDIO_PACKETS = 200;
};

class VideoDecoder : public QThread {
    Q_OBJECT

public:
    explicit VideoDecoder(FFmpegStream *parent);
    ~VideoDecoder();

    void setCodecContext(AVCodecContext *ctx);
    void setDemuxThread(DemuxThread *demux);
    void requestStop();

    bool getFrame(std::unique_ptr<FrameData> &frame);
    bool isFrameQueueFull() const;

protected:
    void run() override;

signals:
    void errorOccurred(const QString &error);

private:
    FFmpegStream *m_parent;
    AVCodecContext *m_codecContext{nullptr};
    DemuxThread *m_demuxThread{nullptr};

    std::atomic<bool> m_stopRequested{false};

    // 帧队列
    std::deque<std::unique_ptr<FrameData>> m_frameQueue;
    mutable QMutex m_frameMutex;
    QWaitCondition m_frameCondition;

    static const int MAX_FRAMES = 30;
};

class AudioDecoder : public QThread {
    Q_OBJECT

public:
    explicit AudioDecoder(FFmpegStream *parent);
    ~AudioDecoder();

    void setCodecContext(AVCodecContext *ctx);
    void setDemuxThread(DemuxThread *demux);
    void requestStop();

    bool getFrame(std::unique_ptr<FrameData> &frame);
    bool isFrameQueueFull() const;

protected:
    void run() override;

signals:
    void errorOccurred(const QString &error);

private:
    FFmpegStream *m_parent;
    AVCodecContext *m_codecContext{nullptr};
    DemuxThread *m_demuxThread{nullptr};
    SwrContext *m_swrContext{nullptr};

    std::atomic<bool> m_stopRequested{false};

    // 帧队列
    std::deque<std::unique_ptr<FrameData>> m_frameQueue;
    mutable QMutex m_frameMutex;
    QWaitCondition m_frameCondition;

    static const int MAX_FRAMES = 100;
};

class FrameCache : public QObject {
    Q_OBJECT

public:
    explicit FrameCache(QObject *parent = nullptr);
    ~FrameCache();

    void setDecoders(VideoDecoder *video, AudioDecoder *audio);

    AVFrame *getNextVideoFrame(double *pts = nullptr);
    AVFrame *getNextAudioFrame(double *pts = nullptr);

    int getVideoFrameCount() const;
    int getAudioFrameCount() const;

    void clear();

private:
    VideoDecoder *m_videoDecoder{nullptr};
    AudioDecoder *m_audioDecoder{nullptr};
};
