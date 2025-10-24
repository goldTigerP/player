#include "media/FFmpegVideoStream.h"
#include <QDebug>

void FFmpegVideoStream::loadVideo(const QString &filePath) {
    const char *filePathStr = filePath.toUtf8().constData();

    int errorCode = 0;
    errorCode = avformat_open_input(&m_formatContext, filePathStr, nullptr, nullptr);
    if (errorCode != 0) {
        qDebug() << "打开视频文件：" << filePath << "失败！错误码：" << errorCode;
        return;
    }

    for (int i = 0; i < m_formatContext->nb_streams; ++i) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = i;
            break;
        }
    }
    if (m_videoStreamIndex == -1) {
        qDebug() << "解析文件失败！未找到视频流！";
        return;
    }

    auto &stream = m_formatContext->streams[m_videoStreamIndex];
    m_codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!m_codec) {
        qDebug() << "查找解码器失败！解码器id：" << int(stream->codecpar->codec_id);
    }

    m_codecContext = avcodec_alloc_context3(m_codec);
    errorCode = avcodec_parameters_to_context(m_codecContext, stream->codecpar);
    if (errorCode != 0) {
        qDebug() << "拷贝编码器参数失败！错误码：" << errorCode;
        return;
    }

    errorCode = avcodec_open2(m_codecContext, m_codec, nullptr);
    if (errorCode != 0) {
        qDebug() << "打开编码器失败！错误码：" << errorCode;
        return;
    }

    // 获取时长
    if (m_formatContext->duration != AV_NOPTS_VALUE) {
        m_duration = double(m_formatContext->duration) / AV_TIME_BASE;
    }

    qDebug() << "视频信息:";
    qDebug() << "分辨率:" << m_codecContext->width << "x" << m_codecContext->height;
    qDebug() << "时长:" << m_duration << "秒";

    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
}

AVFrame *FFmpegVideoStream::getPreviewImage() { return getNextFrame(); }

AVFrame *FFmpegVideoStream::getNextFrame() {
    if (m_packet == nullptr) {
        m_packet = av_packet_alloc();
    }

    if (m_frame == nullptr) {
        m_frame = av_frame_alloc();
    }

    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        if (m_packet->stream_index != m_videoStreamIndex) {
            av_packet_unref(m_packet);
            continue;
        }

        int sendRet = avcodec_send_packet(m_codecContext, m_packet);
        av_packet_unref(m_packet);
        if (sendRet < 0) {
            return nullptr;
        }

        while (true) {
            int recvRet = avcodec_receive_frame(m_codecContext, m_frame);
            if (recvRet == 0) {
                AVFrame *out = av_frame_clone(m_frame);
                av_frame_unref(m_frame);
                return out;
            } else if (recvRet == AVERROR(EAGAIN)) {
                break;
            } else {
                return nullptr;
            }
        }
    }

    return nullptr;
}

double FFmpegVideoStream::getFps() {
    if (m_formatContext == nullptr) {
        return 0;
    }
    auto stream = m_formatContext->streams[m_videoStreamIndex];
    if (stream->avg_frame_rate.num > 0 && stream->avg_frame_rate.den > 0) {
        auto avgRate = av_q2d(stream->avg_frame_rate);
        if (avgRate != 0) {
            qDebug() << "视频帧率:" << avgRate;
            return avgRate;
        }
    }

    if (stream->r_frame_rate.num > 0 && stream->r_frame_rate.den > 0) {
        auto rRate = av_q2d(stream->r_frame_rate);
        if (rRate != 0) {
            qDebug() << "视频帧率:" << rRate;
            return rRate;
        }
    }

    qDebug() << "获取视频帧率失败，设置为默认值 30";
    return 30;
}