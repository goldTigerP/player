#include "media/FFmpegVideoStream.h"
#include <QDebug>

void FFmpegVideoStream::loadVideo(const QString &filePath) {
    qDebug() << "stream try loadVideo " << filePath;

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
    qDebug() << "视频流索引：" << m_videoStreamIndex;

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

    qDebug() << "视频加载成功:" << filePath;
    qDebug() << "分辨率:" << m_codecContext->width << "x" << m_codecContext->height;
    qDebug() << "时长:" << m_duration << "秒";

    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();

    qDebug() << "stream loadVideo over";
}

AVFrame *FFmpegVideoStream::getPreviewImage() {
    qDebug() << "stream getPreviewImage";
    return getNextFrame();
    /*

    int attachedIndex = -1;
    for (int i = 0; i < m_formatContext->nb_streams; ++i) {
        if (m_formatContext->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            attachedIndex = i;
            break;
        }
    }

    int targetIndex = (attachedIndex == -1) ? m_videoStreamIndex : attachedIndex;
    if (targetIndex == -1) {
        qDebug() << "无视频流";
        return nullptr;
    }

    AVCodecContext *decoder = nullptr;
    bool freeDecoder = false;

    if (attachedIndex != -1) {
        AVStream *s = m_formatContext->streams[targetIndex];
        const AVCodec *c = avcodec_find_decoder(s->codecpar->codec_id);
        if (!c) {
            qDebug() << "封面解码器不存在 codec_id:" << s->codecpar->codec_id;
            return nullptr;
        }
        decoder = avcodec_alloc_context3(c);
        if (!decoder) {
            qDebug() << "创建封面解码器上下文失败 codec_id:" << s->codecpar->codec_id;
            return nullptr;
        }
        if (avcodec_parameters_to_context(decoder, s->codecpar) < 0 ||
            avcodec_open2(decoder, c, nullptr) < 0) {
            qDebug() << "打开封面解码器上下文失败 codec_id:" << s->codecpar->codec_id;
            avcodec_free_context(&decoder);
            return nullptr;
        }
        freeDecoder = true;

        // 直接用 attached_pic
        AVPacket *picPkt = &s->attached_pic;
        if (picPkt->data && picPkt->size > 0) {
            if (avcodec_send_packet(decoder, picPkt) < 0) {
                avcodec_free_context(&decoder);
                return nullptr;
            }
            AVFrame *tmp = av_frame_alloc();
            if (!tmp) {
                avcodec_free_context(&decoder);
                return nullptr;
            }
            while (true) {
                int r = avcodec_receive_frame(decoder, tmp);
                if (r == 0) {
                    AVFrame *out = av_frame_clone(tmp);
                    av_frame_free(&tmp);
                    avcodec_free_context(&decoder);
                    return out;
                } else {
                    break;
                }
            }
            av_frame_free(&tmp);
            avcodec_free_context(&decoder);
        } else {
            // 没有有效 attached_pic，退回视频第一帧
            avcodec_free_context(&decoder);
            freeDecoder = false;
            decoder = m_codecContext;
            targetIndex = m_videoStreamIndex;
        }
    } else {
        // 视频第一帧
        decoder = m_codecContext;
    }

    // 2. 读取视频第一帧
    AVFrame *tmpFrame = av_frame_alloc();
    if (!tmpFrame) return nullptr;

    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        if (m_packet->stream_index != targetIndex) {
            av_packet_unref(m_packet);
            continue;
        }
        int sendRet = avcodec_send_packet(decoder, m_packet);
        av_packet_unref(m_packet);
        if (sendRet < 0) {
            break;
        }

        bool end = false;
        while (true) {
            int recvRet = avcodec_receive_frame(decoder, tmpFrame);
            if (recvRet == 0) {
                AVFrame *out = av_frame_clone(tmpFrame);
                av_frame_unref(tmpFrame);
                av_frame_free(&tmpFrame);
                // 回到开头以免影响后续播放
                av_seek_frame(m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);
                avcodec_flush_buffers(m_codecContext);
                return out;
            } else if (recvRet == AVERROR(EAGAIN)) {
                // 需要更多包，跳出内层循环继续读
                break;
            } else {
                // 其他错误
                end = true;
            }
        }
        if (end) {
            break;
        }
    }

    av_frame_free(&tmpFrame);
    av_seek_frame(m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(m_codecContext);
    return nullptr;
    */
}

AVFrame *FFmpegVideoStream::getNextFrame() {
    if (m_packet == nullptr) {
        m_packet = av_packet_alloc();
    }

    if (m_frame == nullptr) {
        m_frame = av_frame_alloc();
    }

    qDebug() << "stream begin get next frame";
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