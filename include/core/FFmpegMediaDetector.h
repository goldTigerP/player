#pragma once

#include <QDebug>
#include <QString>
#include <QStringList>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}


enum MediaType {
    Unknown = 0,
    Video = 1,
    Audio = 2,
    Image = 3,
    Subtitle = 4,
    InvalidFile = -1,
};
struct StreamInfo {
    int index;
    MediaType type;
    AVCodecID codecId;
    QString codecName;
    QString streamTypeString;

    // 视频流特有信息
    int width = 0;
    int height = 0;
    double fps = 0.0;

    // 音频流特有信息
    int sampleRate = 0;
    int channels = 0;
    int bitDepth = 0;

    // 通用信息
    int64_t bitrate = 0;
    bool isAttachedPic = false;
};

struct MediaInfo {
    MediaType primaryType = Unknown;
    QString formatName;
    QString fileName;
    int64_t duration = 0;  // 微秒
    int64_t fileSize = 0;  // 字节
    QList<StreamInfo> streams;

    // 快速访问
    bool hasVideo = false;
    bool hasAudio = false;
    bool hasImage = false;
    bool hasSubtitle = false;

    // 统计信息
    int videoStreamCount = 0;
    int audioStreamCount = 0;
    int imageStreamCount = 0;
    int subtitleStreamCount = 0;
};


class FFmpegMediaDetector {
  public:
    // 🎯 主要检测方法
    static MediaType detectMediaType(const QString& filePath);
    static MediaInfo getDetailedMediaInfo(const QString& filePath);
    static bool isValidMediaFile(const QString& filePath);

    // 🔍 编解码器分类方法
    static MediaType getMediaTypeByCodecId(AVCodecID codecId);
    static bool isVideoCodec(AVCodecID codecId);
    static bool isAudioCodec(AVCodecID codecId);
    static bool isImageCodec(AVCodecID codecId);
    static bool isSubtitleCodec(AVCodecID codecId);

    // 🛠️ 工具方法
    static QString mediaTypeToString(MediaType type);
    static QString codecIdToString(AVCodecID codecId);
    static QString formatDuration(int64_t microseconds);
    static QString formatFileSize(int64_t bytes);
    static QStringList getSupportedVideoCodecs();
    static QStringList getSupportedAudioCodecs();
    static QStringList getSupportedImageCodecs();

    // 🔧 调试方法
    static void printMediaInfo(const MediaInfo& info);
    static void enableDebugOutput(bool enable);

  private:
    static MediaType analyzeStreamsByCodecId(AVFormatContext* formatContext,
                                             MediaInfo* detailInfo = nullptr);
    static StreamInfo extractStreamInfo(AVFormatContext* formatContext, int streamIndex);
    static void updateMediaInfoStatistics(MediaInfo& info);

    static bool s_debugEnabled;
};
