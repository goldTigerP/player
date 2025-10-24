#pragma once

#include <QString>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
}

class FFmpegVideoStream {
public:
    void loadVideo(const QString &filePath);
    AVFrame *getPreviewImage();
    AVFrame *getNextFrame();
    double getFps();

private:
    AVFormatContext *m_formatContext{nullptr};
    int m_videoStreamIndex{-1};
    AVCodec *m_codec{nullptr};
    AVCodecContext *m_codecContext{nullptr};
    AVPacket *m_packet{nullptr};
    AVFrame *m_frame{nullptr};
    double m_duration{};
};
