#include "media/MediaPlayer.h"
#include <QDebug>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>

MediaPlayer::MediaPlayer(QObject* parent)
    : QObject(parent)
    , m_formatContext(nullptr)
    , m_videoCodecContext(nullptr)
    , m_audioCodecContext(nullptr)
    , m_swsContext(nullptr)
    , m_swrContext(nullptr)
    , m_videoStreamIndex(-1)
    , m_audioStreamIndex(-1)
    , m_state(PlayState::Stopped)
    , m_currentPosition(0)
    , m_volume(100)
    , m_muted(false)
    , m_playbackRate(1.0)
    , m_positionTimer(new QTimer(this))
{
    initializeFFmpeg();
    
    // 设置位置更新定时器
    m_positionTimer->setInterval(100); // 100ms更新一次
    connect(m_positionTimer, &QTimer::timeout, this, &MediaPlayer::updatePosition);
}

MediaPlayer::~MediaPlayer() {
    cleanup();
}

bool MediaPlayer::initializeFFmpeg() {
    // FFmpeg 4.0+ 不需要显式注册编解码器
    return true;
}

void MediaPlayer::cleanup() {
    stop();
    
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    
    if (m_swrContext) {
        swr_free(&m_swrContext);
    }
    
    if (m_videoCodecContext) {
        avcodec_free_context(&m_videoCodecContext);
    }
    
    if (m_audioCodecContext) {
        avcodec_free_context(&m_audioCodecContext);
    }
    
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
    }
}

bool MediaPlayer::openMedia(const QString& filePath) {
    cleanup();
    
    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        emit error(QString("File does not exist: %1").arg(filePath));
        return false;
    }
    
    // 打开输入文件
    const char* filename = filePath.toUtf8().constData();
    if (avformat_open_input(&m_formatContext, filename, nullptr, nullptr) != 0) {
        emit error(QString("Could not open file: %1").arg(filePath));
        return false;
    }
    
    // 查找流信息
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        emit error("Could not find stream information");
        cleanup();
        return false;
    }
    
    // 查找视频和音频流
    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        AVCodecParameters* codecpar = m_formatContext->streams[i]->codecpar;
        
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO && m_videoStreamIndex == -1) {
            m_videoStreamIndex = i;
        } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO && m_audioStreamIndex == -1) {
            m_audioStreamIndex = i;
        }
    }
    
    // 打开视频编解码器
    if (m_videoStreamIndex >= 0) {
        if (!openVideoCodec(m_formatContext->streams[m_videoStreamIndex])) {
            qWarning() << "Could not open video codec";
        }
    }
    
    // 打开音频编解码器
    if (m_audioStreamIndex >= 0) {
        if (!openAudioCodec(m_formatContext->streams[m_audioStreamIndex])) {
            qWarning() << "Could not open audio codec";
        }
    }
    
    // 提取媒体信息
    m_mediaInfo.filePath = filePath;
    extractMediaInfo();
    
    emit mediaInfoChanged(m_mediaInfo);
    emit durationChanged(m_mediaInfo.duration);
    
    return true;
}

bool MediaPlayer::openVideoCodec(AVStream* stream) {
    AVCodecParameters* codecpar = stream->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    
    if (!codec) {
        qWarning() << "Video codec not found";
        return false;
    }
    
    m_videoCodecContext = avcodec_alloc_context3(codec);
    if (!m_videoCodecContext) {
        qWarning() << "Could not allocate video codec context";
        return false;
    }
    
    if (avcodec_parameters_to_context(m_videoCodecContext, codecpar) < 0) {
        qWarning() << "Could not copy video codec parameters to context";
        return false;
    }
    
    if (avcodec_open2(m_videoCodecContext, codec, nullptr) < 0) {
        qWarning() << "Could not open video codec";
        return false;
    }
    
    return true;
}

bool MediaPlayer::openAudioCodec(AVStream* stream) {
    AVCodecParameters* codecpar = stream->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    
    if (!codec) {
        qWarning() << "Audio codec not found";
        return false;
    }
    
    m_audioCodecContext = avcodec_alloc_context3(codec);
    if (!m_audioCodecContext) {
        qWarning() << "Could not allocate audio codec context";
        return false;
    }
    
    if (avcodec_parameters_to_context(m_audioCodecContext, codecpar) < 0) {
        qWarning() << "Could not copy audio codec parameters to context";
        return false;
    }
    
    if (avcodec_open2(m_audioCodecContext, codec, nullptr) < 0) {
        qWarning() << "Could not open audio codec";
        return false;
    }
    
    return true;
}

void MediaPlayer::extractMediaInfo() {
    m_mediaInfo.type = detectMediaType(m_mediaInfo.filePath);
    
    if (m_formatContext) {
        // 获取时长
        m_mediaInfo.duration = (m_formatContext->duration != AV_NOPTS_VALUE) 
            ? m_formatContext->duration / AV_TIME_BASE * 1000 : 0;
        
        // 获取比特率
        m_mediaInfo.bitrate = m_formatContext->bit_rate;
        
        // 获取视频信息
        if (m_videoStreamIndex >= 0 && m_videoCodecContext) {
            m_mediaInfo.width = m_videoCodecContext->width;
            m_mediaInfo.height = m_videoCodecContext->height;
            
            AVRational timebase = m_formatContext->streams[m_videoStreamIndex]->time_base;
            AVRational framerate = m_formatContext->streams[m_videoStreamIndex]->r_frame_rate;
            
            if (framerate.num && framerate.den) {
                m_mediaInfo.fps = framerate.num / framerate.den;
            }
            
            const AVCodec* codec = avcodec_find_decoder(m_videoCodecContext->codec_id);
            if (codec) {
                m_mediaInfo.codec = QString(codec->name);
            }
        }
    }
}

MediaType MediaPlayer::detectMediaType(const QString& filePath) {
    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForFile(filePath);
    QString mimeTypeName = mimeType.name();
    
    if (mimeTypeName.startsWith("video/")) {
        return MediaType::Video;
    } else if (mimeTypeName.startsWith("audio/")) {
        return MediaType::Audio;
    } else if (mimeTypeName.startsWith("image/")) {
        return MediaType::Image;
    }
    
    return MediaType::Unknown;
}

void MediaPlayer::play() {
    if (m_state == PlayState::Paused) {
        m_state = PlayState::Playing;
        m_positionTimer->start();
        emit stateChanged(m_state);
    } else if (m_state == PlayState::Stopped && m_formatContext) {
        m_state = PlayState::Playing;
        m_currentPosition = 0;
        m_positionTimer->start();
        emit stateChanged(m_state);
    }
}

void MediaPlayer::pause() {
    if (m_state == PlayState::Playing) {
        m_state = PlayState::Paused;
        m_positionTimer->stop();
        emit stateChanged(m_state);
    }
}

void MediaPlayer::stop() {
    if (m_state != PlayState::Stopped) {
        m_state = PlayState::Stopped;
        m_currentPosition = 0;
        m_positionTimer->stop();
        emit stateChanged(m_state);
        emit positionChanged(m_currentPosition);
    }
}

void MediaPlayer::seek(int64_t position) {
    if (m_formatContext && position >= 0 && position <= m_mediaInfo.duration) {
        m_currentPosition = position;
        
        // TODO: 实现实际的搜索功能
        
        emit positionChanged(m_currentPosition);
    }
}

void MediaPlayer::setVolume(int volume) {
    m_volume = qBound(0, volume, 100);
    // TODO: 实现音量控制
}

void MediaPlayer::setMuted(bool muted) {
    m_muted = muted;
    // TODO: 实现静音控制
}

void MediaPlayer::setPlaybackRate(double rate) {
    m_playbackRate = qBound(0.25, rate, 4.0);
    // TODO: 实现播放速度控制
}

void MediaPlayer::updatePosition() {
    if (m_state == PlayState::Playing) {
        m_currentPosition += 100; // 简单的时间推进
        
        if (m_currentPosition >= m_mediaInfo.duration) {
            m_currentPosition = m_mediaInfo.duration;
            stop();
        }
        
        emit positionChanged(m_currentPosition);
    }
}