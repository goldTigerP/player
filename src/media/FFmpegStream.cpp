#include "media/FFmpegStream.h"
#include <QApplication>
#include <QDebug>

FFmpegStream::FFmpegStream(QObject *parent) : QObject(parent) {
    m_frameCache = std::make_unique<FrameCache>(this);
}

FFmpegStream::~FFmpegStream() { cleanup(); }

bool FFmpegStream::loadVideo(const QString &filePath) {
    cleanup();

    m_filePath = filePath;
    const char *filePathStr = filePath.toUtf8().constData();

    // 打开文件
    int ret = avformat_open_input(&m_formatContext, filePathStr, nullptr, nullptr);
    if (ret != 0) {
        qDebug() << "打开视频文件失败：" << filePath << "错误码：" << ret;
        emit errorOccurred(QString("无法打开文件: %1").arg(filePath));
        return false;
    }

    // 查找流信息
    ret = avformat_find_stream_info(m_formatContext, nullptr);
    if (ret < 0) {
        qDebug() << "查找流信息失败，错误码：" << ret;
        emit errorOccurred("无法获取流信息");
        return false;
    }

    // 初始化流信息
    if (!initializeStreams()) {
        emit errorOccurred("初始化音视频流失败");
        return false;
    }

    // 获取基本信息
    if (m_formatContext->duration != AV_NOPTS_VALUE) {
        m_duration = double(m_formatContext->duration) / AV_TIME_BASE;
    }

    qDebug() << "=== 媒体文件信息 ===";
    qDebug() << "文件:" << filePath;
    qDebug() << "时长:" << m_duration << "秒";
    if (m_hasVideo) {
        qDebug() << "视频分辨率:" << m_width << "x" << m_height;
        qDebug() << "视频帧率:" << m_fps;
    }
    qDebug() << "包含视频:" << m_hasVideo;
    qDebug() << "包含音频:" << m_hasAudio;

    // 启动工作线程
    startThreads();

    m_isLoaded = true;
    emit loadFinished(true);
    return true;
}

AVFrame *FFmpegStream::getNextVideoFrame(double *pts) {
    if (!m_isLoaded || !m_hasVideo || !m_frameCache) {
        return nullptr;
    }
    return m_frameCache->getNextVideoFrame(pts);
}

AVFrame *FFmpegStream::getNextAudioFrame(double *pts) {
    if (!m_isLoaded || !m_hasAudio || !m_frameCache) {
        return nullptr;
    }
    return m_frameCache->getNextAudioFrame(pts);
}

AVFrame *FFmpegStream::getPreviewImage() { return getNextVideoFrame(); }

void FFmpegStream::play() {
    if (!m_isLoaded) return;
    m_isPlaying = true;
}

void FFmpegStream::pause() { m_isPlaying = false; }

void FFmpegStream::stop() {
    m_isPlaying = false;
    stopThreads();
}

void FFmpegStream::seek(double seconds) {
    if (!m_isLoaded || !m_demuxThread) return;
    m_demuxThread->seek(seconds);
}

int FFmpegStream::getVideoFramesInCache() const {
    return m_frameCache ? m_frameCache->getVideoFrameCount() : 0;
}

int FFmpegStream::getAudioFramesInCache() const {
    return m_frameCache ? m_frameCache->getAudioFrameCount() : 0;
}

AVCodecContext *FFmpegStream::getAudioCodecContext() const { return m_audioCodecContext; }

void FFmpegStream::cleanup() {
    stopThreads();

    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }

    if (m_audioCodecContext) {
        avcodec_free_context(&m_audioCodecContext);
        m_audioCodecContext = nullptr;
    }

    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    m_hasVideo = false;
    m_hasAudio = false;
    m_isLoaded = false;
    m_isPlaying = false;

    if (m_frameCache) {
        m_frameCache->clear();
    }
}

bool FFmpegStream::initializeStreams() {
    // 查找视频和音频流
    for (unsigned int i = 0; i < m_formatContext->nb_streams; ++i) {
        AVStream *stream = m_formatContext->streams[i];

        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && m_videoStreamIndex == -1) {
            m_videoStreamIndex = i;
            m_hasVideo = true;

            // 获取视频信息
            m_width = stream->codecpar->width;
            m_height = stream->codecpar->height;

            // 计算帧率
            AVRational frameRate = stream->avg_frame_rate;
            if (frameRate.num > 0 && frameRate.den > 0) {
                m_fps = double(frameRate.num) / frameRate.den;
            } else {
                frameRate = stream->r_frame_rate;
                if (frameRate.num > 0 && frameRate.den > 0) {
                    m_fps = double(frameRate.num) / frameRate.den;
                }
            }
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && m_audioStreamIndex == -1) {
            m_audioStreamIndex = i;
            m_hasAudio = true;
        }
    }

    return m_hasVideo || m_hasAudio;
}

void FFmpegStream::startThreads() {
    // 创建解封装线程
    m_demuxThread = std::make_unique<DemuxThread>(this);
    m_demuxThread->setFormatContext(m_formatContext, m_videoStreamIndex, m_audioStreamIndex);

    // 创建解码线程
    if (m_hasVideo) {
        AVStream *videoStream = m_formatContext->streams[m_videoStreamIndex];
        const AVCodec *videoCodec = avcodec_find_decoder(videoStream->codecpar->codec_id);
        if (videoCodec) {
            AVCodecContext *videoCtx = avcodec_alloc_context3(videoCodec);
            avcodec_parameters_to_context(videoCtx, videoStream->codecpar);
            if (avcodec_open2(videoCtx, videoCodec, nullptr) >= 0) {
                m_videoDecoder = std::make_unique<VideoDecoder>(this);
                m_videoDecoder->setCodecContext(videoCtx);
                m_videoDecoder->setDemuxThread(m_demuxThread.get());
            }
        }
    }

    if (m_hasAudio) {
        AVStream *audioStream = m_formatContext->streams[m_audioStreamIndex];
        const AVCodec *audioCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
        if (audioCodec) {
            m_audioCodecContext = avcodec_alloc_context3(audioCodec);
            avcodec_parameters_to_context(m_audioCodecContext, audioStream->codecpar);
            if (avcodec_open2(m_audioCodecContext, audioCodec, nullptr) >= 0) {
                m_audioDecoder = std::make_unique<AudioDecoder>(this);
                m_audioDecoder->setCodecContext(m_audioCodecContext);
                m_audioDecoder->setDemuxThread(m_demuxThread.get());
            }
        }
    }

    // 设置帧缓存的解码器引用
    m_frameCache->setDecoders(m_videoDecoder.get(), m_audioDecoder.get());

    // 启动线程
    if (m_demuxThread) m_demuxThread->start();
    if (m_videoDecoder) m_videoDecoder->start();
    if (m_audioDecoder) m_audioDecoder->start();
}

void FFmpegStream::stopThreads() {
    // 请求所有线程停止
    if (m_demuxThread) {
        m_demuxThread->requestStop();
        m_demuxThread->wait(3000);
    }

    if (m_videoDecoder) {
        m_videoDecoder->requestStop();
        m_videoDecoder->wait(3000);
    }

    if (m_audioDecoder) {
        m_audioDecoder->requestStop();
        m_audioDecoder->wait(3000);
    }

    // 清理
    m_demuxThread.reset();
    m_videoDecoder.reset();
    m_audioDecoder.reset();
}

void FFmpegStream::onDemuxFinished() { emit endOfStream(); }

void FFmpegStream::onVideoDecodeError() { emit errorOccurred("视频解码错误"); }

void FFmpegStream::onAudioDecodeError() { emit errorOccurred("音频解码错误"); }

// ============== DemuxThread 解封装线程实现 ==============

DemuxThread::DemuxThread(FFmpegStream *parent) : QThread(parent), m_parent(parent) {}

DemuxThread::~DemuxThread() {
    requestStop();
    wait();
}

void DemuxThread::setFormatContext(AVFormatContext *ctx, int videoIndex, int audioIndex) {
    m_formatContext = ctx;
    m_videoStreamIndex = videoIndex;
    m_audioStreamIndex = audioIndex;
}

void DemuxThread::requestStop() {
    m_stopRequested = true;

    // 唤醒所有等待的线程
    m_videoCondition.wakeAll();
    m_audioCondition.wakeAll();
}

void DemuxThread::seek(double seconds) {
    m_seekTime = seconds;
    m_seekRequested = true;
}

void DemuxThread::run() {
    if (!m_formatContext) {
        emit errorOccurred("格式上下文为空");
        return;
    }

    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        emit errorOccurred("分配数据包失败");
        return;
    }

    while (!m_stopRequested) {
        // 处理跳转请求
        if (m_seekRequested) {
            int64_t seekTarget = int64_t(m_seekTime * AV_TIME_BASE);
            if (av_seek_frame(m_formatContext, -1, seekTarget, AVSEEK_FLAG_BACKWARD) >= 0) {
                // 清空队列
                {
                    QMutexLocker videoLocker(&m_videoMutex);
                    m_videoPacketQueue.clear();
                }
                {
                    QMutexLocker audioLocker(&m_audioMutex);
                    m_audioPacketQueue.clear();
                }
            }
            m_seekRequested = false;
        }

        // 检查队列是否已满
        bool videoFull = (m_videoStreamIndex >= 0) && isVideoQueueFull();
        bool audioFull = (m_audioStreamIndex >= 0) && isAudioQueueFull();

        if (videoFull && audioFull) {
            // 两个队列都满了，等待一下
            QThread::msleep(10);
            continue;
        }

        // 读取数据包
        int ret = av_read_frame(m_formatContext, packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                qDebug() << "解封装完成，到达文件末尾";
                emit finished();
            } else {
                emit errorOccurred(QString("读取数据包失败，错误码: %1").arg(ret));
            }
            break;
        }

        // 计算时间戳
        double pts = 0.0;
        if (packet->pts != AV_NOPTS_VALUE) {
            AVStream *stream = m_formatContext->streams[packet->stream_index];
            pts = packet->pts * av_q2d(stream->time_base);
        }

        // 分发数据包
        if (packet->stream_index == m_videoStreamIndex && !videoFull) {
            // 视频数据包
            AVPacket *videoPkt = av_packet_clone(packet);
            auto packetData = std::make_unique<PacketData>(videoPkt, pts);

            QMutexLocker locker(&m_videoMutex);
            m_videoPacketQueue.push_back(std::move(packetData));
            m_videoCondition.wakeOne();
        } else if (packet->stream_index == m_audioStreamIndex && !audioFull) {
            // 音频数据包
            AVPacket *audioPkt = av_packet_clone(packet);
            auto packetData = std::make_unique<PacketData>(audioPkt, pts);

            QMutexLocker locker(&m_audioMutex);
            m_audioPacketQueue.push_back(std::move(packetData));
            m_audioCondition.wakeOne();
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    qDebug() << "解封装线程退出";
}

bool DemuxThread::getVideoPacket(std::unique_ptr<PacketData> &packet) {
    QMutexLocker locker(&m_videoMutex);

    while (m_videoPacketQueue.empty() && !m_stopRequested) {
        m_videoCondition.wait(&m_videoMutex, 100);
    }

    if (!m_videoPacketQueue.empty()) {
        packet = std::move(m_videoPacketQueue.front());
        m_videoPacketQueue.pop_front();
        return true;
    }

    return false;
}

bool DemuxThread::getAudioPacket(std::unique_ptr<PacketData> &packet) {
    QMutexLocker locker(&m_audioMutex);

    while (m_audioPacketQueue.empty() && !m_stopRequested) {
        m_audioCondition.wait(&m_audioMutex, 100);
    }

    if (!m_audioPacketQueue.empty()) {
        packet = std::move(m_audioPacketQueue.front());
        m_audioPacketQueue.pop_front();
        return true;
    }

    return false;
}

bool DemuxThread::isVideoQueueFull() const {
    QMutexLocker locker(&m_videoMutex);
    return m_videoPacketQueue.size() >= MAX_VIDEO_PACKETS;
}

bool DemuxThread::isAudioQueueFull() const {
    QMutexLocker locker(&m_audioMutex);
    return m_audioPacketQueue.size() >= MAX_AUDIO_PACKETS;
}

// ============== VideoDecoder 视频解码器实现 ==============

VideoDecoder::VideoDecoder(FFmpegStream *parent) : QThread(parent), m_parent(parent) {}

VideoDecoder::~VideoDecoder() {
    requestStop();
    wait();
}

void VideoDecoder::setCodecContext(AVCodecContext *ctx) { m_codecContext = ctx; }

void VideoDecoder::setDemuxThread(DemuxThread *demux) { m_demuxThread = demux; }

void VideoDecoder::requestStop() {
    m_stopRequested = true;
    m_frameCondition.wakeAll();
}

void VideoDecoder::run() {
    if (!m_codecContext || !m_demuxThread) {
        emit errorOccurred("视频解码器初始化失败");
        return;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        emit errorOccurred("分配视频帧失败");
        return;
    }

    while (!m_stopRequested) {
        // 检查帧队列是否已满
        if (isFrameQueueFull()) {
            QThread::msleep(10);
            continue;
        }

        // 获取视频数据包
        std::unique_ptr<PacketData> packetData;
        if (!m_demuxThread->getVideoPacket(packetData)) {
            continue;  // 没有数据包或被请求停止
        }

        // 发送数据包到解码器
        int ret = avcodec_send_packet(m_codecContext, packetData->packet);
        if (ret < 0) {
            qDebug() << "发送视频包到解码器失败：" << ret;
            continue;
        }

        // 接收解码后的帧
        while (ret >= 0 && !m_stopRequested) {
            ret = avcodec_receive_frame(m_codecContext, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                qDebug() << "视频解码失败：" << ret;
                break;
            }

            // 克隆帧数据
            AVFrame *clonedFrame = av_frame_clone(frame);
            if (clonedFrame) {
                double pts = packetData->pts;
                if (frame->pts != AV_NOPTS_VALUE) {
                    pts = frame->pts * av_q2d(m_codecContext->time_base);
                }

                auto frameData = std::make_unique<FrameData>(clonedFrame, pts);

                // 添加到队列
                QMutexLocker locker(&m_frameMutex);
                m_frameQueue.push_back(std::move(frameData));
                m_frameCondition.wakeOne();
            }

            av_frame_unref(frame);
        }
    }

    av_frame_free(&frame);
    qDebug() << "视频解码线程退出";
}

bool VideoDecoder::getFrame(std::unique_ptr<FrameData> &frame) {
    QMutexLocker locker(&m_frameMutex);

    while (m_frameQueue.empty() && !m_stopRequested) {
        m_frameCondition.wait(&m_frameMutex, 100);
    }

    if (!m_frameQueue.empty()) {
        frame = std::move(m_frameQueue.front());
        m_frameQueue.pop_front();
        return true;
    }

    return false;
}

bool VideoDecoder::isFrameQueueFull() const {
    QMutexLocker locker(&m_frameMutex);
    return m_frameQueue.size() >= MAX_FRAMES;
}

// ============== AudioDecoder 音频解码器实现 ==============

AudioDecoder::AudioDecoder(FFmpegStream *parent)
    : QThread(parent), m_parent(parent), m_swrContext(nullptr) {}

AudioDecoder::~AudioDecoder() {
    requestStop();
    wait();

    if (m_swrContext) {
        swr_free(&m_swrContext);
    }
}

void AudioDecoder::setCodecContext(AVCodecContext *ctx) { m_codecContext = ctx; }

void AudioDecoder::setDemuxThread(DemuxThread *demux) { m_demuxThread = demux; }

void AudioDecoder::requestStop() {
    m_stopRequested = true;
    m_frameCondition.wakeAll();
}

void AudioDecoder::run() {
    if (!m_codecContext || !m_demuxThread) {
        emit errorOccurred("音频解码器初始化失败");
        return;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        emit errorOccurred("分配音频帧失败");
        return;
    }

    while (!m_stopRequested) {
        // 检查帧队列是否已满
        if (isFrameQueueFull()) {
            QThread::msleep(10);
            continue;
        }

        // 获取音频数据包
        std::unique_ptr<PacketData> packetData;
        if (!m_demuxThread->getAudioPacket(packetData)) {
            continue;  // 没有数据包或被请求停止
        }

        // 发送数据包到解码器
        int ret = avcodec_send_packet(m_codecContext, packetData->packet);
        if (ret < 0) {
            qDebug() << "发送音频包到解码器失败：" << ret;
            continue;
        }

        // 接收解码后的帧
        while (ret >= 0 && !m_stopRequested) {
            ret = avcodec_receive_frame(m_codecContext, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                qDebug() << "音频解码失败：" << ret;
                break;
            }

            // 克隆帧数据
            AVFrame *clonedFrame = av_frame_clone(frame);
            if (clonedFrame) {
                double pts = packetData->pts;
                if (frame->pts != AV_NOPTS_VALUE) {
                    pts = frame->pts * av_q2d(m_codecContext->time_base);
                }

                auto frameData = std::make_unique<FrameData>(clonedFrame, pts);

                // 添加到队列
                QMutexLocker locker(&m_frameMutex);
                m_frameQueue.push_back(std::move(frameData));
                m_frameCondition.wakeOne();
            }

            av_frame_unref(frame);
        }
    }

    av_frame_free(&frame);
    qDebug() << "音频解码线程退出";
}

bool AudioDecoder::getFrame(std::unique_ptr<FrameData> &frame) {
    QMutexLocker locker(&m_frameMutex);

    while (m_frameQueue.empty() && !m_stopRequested) {
        m_frameCondition.wait(&m_frameMutex, 100);
    }

    if (!m_frameQueue.empty()) {
        frame = std::move(m_frameQueue.front());
        m_frameQueue.pop_front();
        return true;
    }

    return false;
}

bool AudioDecoder::isFrameQueueFull() const {
    QMutexLocker locker(&m_frameMutex);
    return m_frameQueue.size() >= MAX_FRAMES;
}

// ============== FrameCache 帧缓存管理器实现 ==============

FrameCache::FrameCache(QObject *parent) : QObject(parent) {}

FrameCache::~FrameCache() { clear(); }

void FrameCache::setDecoders(VideoDecoder *video, AudioDecoder *audio) {
    m_videoDecoder = video;
    m_audioDecoder = audio;
}

AVFrame *FrameCache::getNextVideoFrame(double *pts) {
    if (!m_videoDecoder) return nullptr;

    std::unique_ptr<FrameData> frameData;
    if (m_videoDecoder->getFrame(frameData)) {
        if (pts) *pts = frameData->pts;

        // 移动帧所有权给调用方
        AVFrame *frame = frameData->frame;
        frameData->frame = nullptr;
        return frame;
    }

    return nullptr;
}

AVFrame *FrameCache::getNextAudioFrame(double *pts) {
    if (!m_audioDecoder) return nullptr;

    std::unique_ptr<FrameData> frameData;
    if (m_audioDecoder->getFrame(frameData)) {
        if (pts) *pts = frameData->pts;

        // 移动帧所有权给调用方
        AVFrame *frame = frameData->frame;
        frameData->frame = nullptr;
        return frame;
    }

    return nullptr;
}

int FrameCache::getVideoFrameCount() const {
    // 这里需要从VideoDecoder获取队列大小
    // 为了简化，暂时返回0
    return 0;
}

int FrameCache::getAudioFrameCount() const {
    // 这里需要从AudioDecoder获取队列大小
    // 为了简化，暂时返回0
    return 0;
}

void FrameCache::clear() {
    // 清理缓存（由各个解码器自己管理队列清理）
}

#include "media/FFmpegStream.moc"