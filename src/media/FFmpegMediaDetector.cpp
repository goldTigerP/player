#include "media/FFmpegMediaDetector.h"
#include <QElapsedTimer>
#include <QFileInfo>

bool FFmpegMediaDetector::s_debugEnabled = true;

// 🎯 主要检测方法 - 基于编解码器ID
MediaType FFmpegMediaDetector::detectMediaType(const QString &filePath) {
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        if (s_debugEnabled) qDebug() << "文件不存在:" << filePath;
        return InvalidFile;
    }

    AVFormatContext *formatContext = nullptr;

    // ⚡ 快速检测模式 - 只读取必要信息
    AVDictionary *options = nullptr;
    av_dict_set(&options, "probesize", "65536", 0);          // 64KB探测
    av_dict_set(&options, "analyzeduration", "1000000", 0);  // 1秒分析

    int ret = avformat_open_input(&formatContext, filePath.toUtf8().constData(), nullptr, &options);
    av_dict_free(&options);

    if (ret != 0) {
        if (s_debugEnabled) {
            qDebug() << "无法打开文件:" << filePath << "错误码:" << ret;
        }
        return InvalidFile;
    }

    // 获取流信息
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0) {
        if (s_debugEnabled) {
            qDebug() << "无法获取流信息:" << filePath << "错误码:" << ret;
        }
        avformat_close_input(&formatContext);
        return InvalidFile;
    }

    MediaType result = analyzeStreamsByCodecId(formatContext);

    if (s_debugEnabled) {
        qDebug() << "检测结果:" << filePath << "->" << mediaTypeToString(result);
    }

    avformat_close_input(&formatContext);
    return result;
}

// 📊 获取详细媒体信息
MediaInfo FFmpegMediaDetector::getDetailedMediaInfo(const QString &filePath) {
    MediaInfo info;
    info.fileName = QFileInfo(filePath).fileName();
    info.fileSize = QFileInfo(filePath).size();

    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        info.primaryType = InvalidFile;
        return info;
    }

    AVFormatContext *formatContext = nullptr;

    int ret = avformat_open_input(&formatContext, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret != 0) {
        info.primaryType = InvalidFile;
        return info;
    }

    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0) {
        info.primaryType = InvalidFile;
        avformat_close_input(&formatContext);
        return info;
    }

    // 基本信息
    info.formatName = QString(formatContext->iformat->name);
    info.duration = formatContext->duration;

    // 分析所有流并获取详细信息
    info.primaryType = analyzeStreamsByCodecId(formatContext, &info);

    // 更新统计信息
    updateMediaInfoStatistics(info);

    avformat_close_input(&formatContext);
    return info;
}

// 🔍 核心方法：基于编解码器ID分析流
MediaType FFmpegMediaDetector::analyzeStreamsByCodecId(AVFormatContext *formatContext,
                                                       MediaInfo *detailInfo) {
    if (!formatContext || formatContext->nb_streams == 0) {
        if (s_debugEnabled) qDebug() << "没有找到媒体流";
        return Unknown;
    }

    MediaType primaryType = Unknown;
    bool hasVideo = false;
    bool hasAudio = false;
    bool hasImage = false;
    bool hasSubtitle = false;

    if (s_debugEnabled) {
        qDebug() << "=== 开始分析" << formatContext->nb_streams << "个流 ===";
    }

    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        AVCodecParameters *codecParams = formatContext->streams[i]->codecpar;
        AVStream *stream = formatContext->streams[i];

        if (!codecParams || codecParams->codec_id == AV_CODEC_ID_NONE) {
            if (s_debugEnabled) qDebug() << "流" << i << ": 编解码器信息无效";
            continue;
        }

        // 获取流信息（如果需要详细信息）
        if (detailInfo) {
            StreamInfo streamInfo = extractStreamInfo(formatContext, i);
            detailInfo->streams.append(streamInfo);
        }

        // 🎯 关键：基于编解码器ID判断媒体类型
        MediaType streamType = getMediaTypeByCodecId(codecParams->codec_id);

        if (s_debugEnabled) {
            const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
            qDebug() << QString("流 %1: 编解码器=%2 (ID=%3) -> 类型=%4")
                            .arg(i)
                            .arg(codec ? codec->name : "未知")
                            .arg(codecParams->codec_id)
                            .arg(mediaTypeToString(streamType));
        }

        // 检查是否为附加图片（如专辑封面）
        bool isAttachedPic = (stream->disposition & AV_DISPOSITION_ATTACHED_PIC);
        if (isAttachedPic && s_debugEnabled) {
            qDebug() << "  -> 跳过附加图片";
            continue;
        }

        // 🎯 设置标志和优先级
        switch (streamType) {
        case Video: hasVideo = true; break;
        case Audio: hasAudio = true; break;
        case Image:
            if (!isAttachedPic) {  // 排除附加图片
                hasImage = true;
            }
            break;
        case Subtitle: hasSubtitle = true; break;
        default: break;
        }
    }

    // 🎯 优先级决策：Video > Image > Audio > Subtitle
    if (hasVideo) {
        primaryType = Video;
    } else if (hasImage) {
        primaryType = Image;
    } else if (hasAudio) {
        primaryType = Audio;
    } else if (hasSubtitle) {
        primaryType = Subtitle;
    }

    if (s_debugEnabled) {
        qDebug() << QString("最终类型: %1 (视频=%2, 图片=%3, 音频=%4, 字幕=%5)")
                        .arg(mediaTypeToString(primaryType))
                        .arg(hasVideo ? "是" : "否")
                        .arg(hasImage ? "是" : "否")
                        .arg(hasAudio ? "是" : "否")
                        .arg(hasSubtitle ? "是" : "否");
    }

    return primaryType;
}

// 🎯 根据编解码器ID获取媒体类型
MediaType FFmpegMediaDetector::getMediaTypeByCodecId(AVCodecID codecId) {
    // 🖼️ 图片编解码器
    if (isImageCodec(codecId)) {
        return Image;
    }

    // 🎬 视频编解码器
    if (isVideoCodec(codecId)) {
        return Video;
    }

    // 🎵 音频编解码器
    if (isAudioCodec(codecId)) {
        return Audio;
    }

    // 📝 字幕编解码器
    if (isSubtitleCodec(codecId)) {
        return Subtitle;
    }

    return Unknown;
}

// 🖼️ 判断是否为图片编解码器
bool FFmpegMediaDetector::isImageCodec(AVCodecID codecId) {
    switch (codecId) {
    // 静态图片格式
    case AV_CODEC_ID_MJPEG:     // JPEG
    case AV_CODEC_ID_PNG:       // PNG
    case AV_CODEC_ID_BMP:       // BMP
    case AV_CODEC_ID_TIFF:      // TIFF
    case AV_CODEC_ID_WEBP:      // WebP
    case AV_CODEC_ID_SVG:       // SVG
    case AV_CODEC_ID_PSD:       // Photoshop
    case AV_CODEC_ID_PCX:       // PCX
    case AV_CODEC_ID_SGI:       // SGI
    case AV_CODEC_ID_SUNRAST:   // Sun Raster
    case AV_CODEC_ID_TARGA:     // Targa
    case AV_CODEC_ID_XBM:       // XBM
    case AV_CODEC_ID_XPM:       // XPM
    case AV_CODEC_ID_XWD:       // XWD
    case AV_CODEC_ID_PICTOR:    // Pictor
    case AV_CODEC_ID_PPM:       // PPM
    case AV_CODEC_ID_PBM:       // PBM
    case AV_CODEC_ID_PGM:       // PGM
    case AV_CODEC_ID_PGMYUV:    // PGMYUV
    case AV_CODEC_ID_PAM:       // PAM
    case AV_CODEC_ID_DPX:       // DPX
    case AV_CODEC_ID_EXR:       // OpenEXR
    case AV_CODEC_ID_JPEGLS:    // JPEG-LS
    case AV_CODEC_ID_JPEG2000:  // JPEG 2000
    case AV_CODEC_ID_GIF:       // GIF (可能是动画)
    case AV_CODEC_ID_APNG:      // 动画PNG
        return true;
    default: return false;
    }
}

// 🎬 判断是否为视频编解码器
bool FFmpegMediaDetector::isVideoCodec(AVCodecID codecId) {
    switch (codecId) {
    // 现代视频编解码器
    case AV_CODEC_ID_H264:        // H.264/AVC
    case AV_CODEC_ID_HEVC:        // H.265/HEVC
    case AV_CODEC_ID_VP8:         // VP8
    case AV_CODEC_ID_VP9:         // VP9
    case AV_CODEC_ID_AV1:         // AV1
    case AV_CODEC_ID_MPEG4:       // MPEG-4
    case AV_CODEC_ID_MPEG2VIDEO:  // MPEG-2
    case AV_CODEC_ID_MPEG1VIDEO:  // MPEG-1
    case AV_CODEC_ID_WMV3:        // WMV3
    case AV_CODEC_ID_VC1:         // VC1
    case AV_CODEC_ID_THEORA:      // Theora
    case AV_CODEC_ID_DIRAC:       // Dirac
    case AV_CODEC_ID_PRORES:      // ProRes
    case AV_CODEC_ID_DNXHD:       // DNxHD
    case AV_CODEC_ID_RAWVIDEO:    // Raw Video
    case AV_CODEC_ID_FFV1:        // FFV1
    case AV_CODEC_ID_HUFFYUV:     // HuffYUV

    // 传统视频编解码器
    case AV_CODEC_ID_CINEPAK:    // Cinepak
    case AV_CODEC_ID_INDEO2:     // Indeo 2
    case AV_CODEC_ID_INDEO3:     // Indeo 3
    case AV_CODEC_ID_INDEO4:     // Indeo 4
    case AV_CODEC_ID_INDEO5:     // Indeo 5
    case AV_CODEC_ID_MSMPEG4V1:  // MS MPEG4 V1
    case AV_CODEC_ID_MSMPEG4V2:  // MS MPEG4 V2
    case AV_CODEC_ID_MSMPEG4V3:  // MS MPEG4 V3
    case AV_CODEC_ID_WMV1:       // WMV1
    case AV_CODEC_ID_WMV2:       // WMV2
    case AV_CODEC_ID_FLV1:       // Flash Video
    case AV_CODEC_ID_H263:       // H.263
    case AV_CODEC_ID_H263P:      // H.263+
        return true;
    default: return false;
    }
}

// 🎵 判断是否为音频编解码器
bool FFmpegMediaDetector::isAudioCodec(AVCodecID codecId) {
    switch (codecId) {
    // 无损音频
    case AV_CODEC_ID_FLAC:     // FLAC
    case AV_CODEC_ID_ALAC:     // Apple Lossless
    case AV_CODEC_ID_APE:      // Monkey's Audio
    case AV_CODEC_ID_WAVPACK:  // WavPack
    case AV_CODEC_ID_TTA:      // True Audio
    case AV_CODEC_ID_SHORTEN:  // Shorten

    // 有损音频
    case AV_CODEC_ID_MP3:          // MP3
    case AV_CODEC_ID_AAC:          // AAC
    case AV_CODEC_ID_AC3:          // AC3
    case AV_CODEC_ID_EAC3:         // Enhanced AC3
    case AV_CODEC_ID_DTS:          // DTS
    case AV_CODEC_ID_TRUEHD:       // TrueHD
    case AV_CODEC_ID_VORBIS:       // Ogg Vorbis
    case AV_CODEC_ID_OPUS:         // Opus
    case AV_CODEC_ID_WMAV1:        // WMA V1
    case AV_CODEC_ID_WMAV2:        // WMA V2
    case AV_CODEC_ID_WMAVOICE:     // WMA voice
    case AV_CODEC_ID_WMALOSSLESS:  // WMA Lossless
    case AV_CODEC_ID_WMAPRO:       // WMA Pro

    // PCM音频
    case AV_CODEC_ID_PCM_S16LE:  // PCM 16-bit LE
    case AV_CODEC_ID_PCM_S16BE:  // PCM 16-bit BE
    case AV_CODEC_ID_PCM_S24LE:  // PCM 24-bit LE
    case AV_CODEC_ID_PCM_S24BE:  // PCM 24-bit BE
    case AV_CODEC_ID_PCM_S32LE:  // PCM 32-bit LE
    case AV_CODEC_ID_PCM_S32BE:  // PCM 32-bit BE
    case AV_CODEC_ID_PCM_F32LE:  // PCM 32-bit float LE
    case AV_CODEC_ID_PCM_F32BE:  // PCM 32-bit float BE
    case AV_CODEC_ID_PCM_F64LE:  // PCM 64-bit float LE
    case AV_CODEC_ID_PCM_F64BE:  // PCM 64-bit float BE
    case AV_CODEC_ID_PCM_U8:     // PCM 8-bit unsigned
    case AV_CODEC_ID_PCM_S8:     // PCM 8-bit signed

    // 其他音频格式
    case AV_CODEC_ID_ADPCM_IMA_WAV:  // ADPCM IMA WAV
    case AV_CODEC_ID_ADPCM_MS:       // ADPCM MS
    case AV_CODEC_ID_GSM:            // GSM
    case AV_CODEC_ID_GSM_MS:         // GSM MS
    case AV_CODEC_ID_AMR_NB:         // AMR Narrowband
    case AV_CODEC_ID_AMR_WB:         // AMR Wideband
        return true;
    default: return false;
    }
}

// 📝 判断是否为字幕编解码器
bool FFmpegMediaDetector::isSubtitleCodec(AVCodecID codecId) {
    switch (codecId) {
    case AV_CODEC_ID_SRT:                 // SubRip
    case AV_CODEC_ID_ASS:                 // Advanced SubStation Alpha
    case AV_CODEC_ID_SSA:                 // SubStation Alpha
    case AV_CODEC_ID_SUBRIP:              // SubRip
    case AV_CODEC_ID_DVD_SUBTITLE:        // DVD字幕
    case AV_CODEC_ID_DVB_SUBTITLE:        // DVB字幕
    case AV_CODEC_ID_TEXT:                // 纯文本字幕
    case AV_CODEC_ID_WEBVTT:              // WebVTT
    case AV_CODEC_ID_PJS:                 // PJS字幕
    case AV_CODEC_ID_HDMV_PGS_SUBTITLE:   // HDMV PGS
    case AV_CODEC_ID_HDMV_TEXT_SUBTITLE:  // HDMV Text
        return true;
    default: return false;
    }
}

// 📊 提取流的详细信息
StreamInfo FFmpegMediaDetector::extractStreamInfo(AVFormatContext *formatContext, int streamIndex) {
    StreamInfo info;
    info.index = streamIndex;

    AVCodecParameters *codecParams = formatContext->streams[streamIndex]->codecpar;
    AVStream *stream = formatContext->streams[streamIndex];

    info.codecId = codecParams->codec_id;
    info.type = getMediaTypeByCodecId(codecParams->codec_id);
    info.isAttachedPic = (stream->disposition & AV_DISPOSITION_ATTACHED_PIC);
    info.bitrate = codecParams->bit_rate;

    // 获取编解码器名称
    const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
    info.codecName = codec ? QString(codec->name) : "未知";

    // 获取流类型字符串
    info.streamTypeString = QString(av_get_media_type_string(codecParams->codec_type));

    // 根据类型提取特定信息
    switch (codecParams->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
        info.width = codecParams->width;
        info.height = codecParams->height;
        if (stream->avg_frame_rate.den != 0) {
            info.fps = av_q2d(stream->avg_frame_rate);
        }
        break;

    case AVMEDIA_TYPE_AUDIO:
        info.sampleRate = codecParams->sample_rate;
        info.channels = codecParams->channels;
        info.bitDepth = codecParams->bits_per_coded_sample;
        break;

    default: break;
    }

    return info;
}

// 📊 更新媒体信息统计
void FFmpegMediaDetector::updateMediaInfoStatistics(MediaInfo &info) {
    info.hasVideo = false;
    info.hasAudio = false;
    info.hasImage = false;
    info.hasSubtitle = false;

    info.videoStreamCount = 0;
    info.audioStreamCount = 0;
    info.imageStreamCount = 0;
    info.subtitleStreamCount = 0;

    for (const StreamInfo &stream : info.streams) {
        if (stream.isAttachedPic) continue;  // 跳过附加图片

        switch (stream.type) {
        case Video:
            info.hasVideo = true;
            info.videoStreamCount++;
            break;
        case Audio:
            info.hasAudio = true;
            info.audioStreamCount++;
            break;
        case Image:
            info.hasImage = true;
            info.imageStreamCount++;
            break;
        case Subtitle:
            info.hasSubtitle = true;
            info.subtitleStreamCount++;
            break;
        default: break;
        }
    }
}

// 🛠️ 工具方法实现
QString FFmpegMediaDetector::mediaTypeToString(MediaType type) {
    switch (type) {
    case Video: return "视频";
    case Audio: return "音频";
    case Image: return "图片";
    case Subtitle: return "字幕";
    case InvalidFile: return "无效文件";
    default: return "未知";
    }
}

QString FFmpegMediaDetector::codecIdToString(AVCodecID codecId) {
    const AVCodec *codec = avcodec_find_decoder(codecId);
    if (codec) {
        return QString(codec->name);
    }
    return QString("未知编解码器(ID:%1)").arg(codecId);
}

QString FFmpegMediaDetector::formatDuration(int64_t microseconds) {
    if (microseconds <= 0) return "未知";

    int seconds = microseconds / 1000000;
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;

    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
    }
}

QString FFmpegMediaDetector::formatFileSize(int64_t bytes) {
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
}

bool FFmpegMediaDetector::isValidMediaFile(const QString &filePath) {
    MediaType type = detectMediaType(filePath);
    return (type != Unknown && type != InvalidFile);
}

// 🔧 调试方法
void FFmpegMediaDetector::printMediaInfo(const MediaInfo &info) {
    qDebug() << "=== 媒体文件信息 ===";
    qDebug() << "文件名:" << info.fileName;
    qDebug() << "主要类型:" << mediaTypeToString(info.primaryType);
    qDebug() << "容器格式:" << info.formatName;
    qDebug() << "文件大小:" << formatFileSize(info.fileSize);
    qDebug() << "时长:" << formatDuration(info.duration);
    qDebug() << "流统计: 视频" << info.videoStreamCount << "音频" << info.audioStreamCount << "图片"
             << info.imageStreamCount << "字幕" << info.subtitleStreamCount;

    qDebug() << "\n--- 流详情 ---";
    for (const StreamInfo &stream : info.streams) {
        QString streamDesc = QString("流 %1: %2 (%3)")
                                 .arg(stream.index)
                                 .arg(stream.codecName)
                                 .arg(mediaTypeToString(stream.type));

        if (stream.type == Video && !stream.isAttachedPic) {
            streamDesc += QString(" %1x%2 %3fps")
                              .arg(stream.width)
                              .arg(stream.height)
                              .arg(stream.fps, 0, 'f', 2);
        } else if (stream.type == Audio) {
            streamDesc += QString(" %1Hz %2声道").arg(stream.sampleRate).arg(stream.channels);
        }

        if (stream.isAttachedPic) {
            streamDesc += " [附加图片]";
        }

        qDebug() << streamDesc;
    }
    qDebug() << "==================";
}

void FFmpegMediaDetector::enableDebugOutput(bool enable) { s_debugEnabled = enable; }

// 🔍 获取支持的编解码器列表
QStringList FFmpegMediaDetector::getSupportedVideoCodecs() {
    QStringList codecs;
    const AVCodec *codec = nullptr;
    void *iter = nullptr;

    while ((codec = av_codec_iterate(&iter))) {
        if (codec->type == AVMEDIA_TYPE_VIDEO && av_codec_is_decoder(codec)) {
            if (isVideoCodec(codec->id)) {
                codecs << QString(codec->name);
            }
        }
    }

    codecs.removeDuplicates();
    codecs.sort();
    return codecs;
}

QStringList FFmpegMediaDetector::getSupportedAudioCodecs() {
    QStringList codecs;
    const AVCodec *codec = nullptr;
    void *iter = nullptr;

    while ((codec = av_codec_iterate(&iter))) {
        if (codec->type == AVMEDIA_TYPE_AUDIO && av_codec_is_decoder(codec)) {
            if (isAudioCodec(codec->id)) {
                codecs << QString(codec->name);
            }
        }
    }

    codecs.removeDuplicates();
    codecs.sort();
    return codecs;
}

QStringList FFmpegMediaDetector::getSupportedImageCodecs() {
    QStringList codecs;
    const AVCodec *codec = nullptr;
    void *iter = nullptr;

    while ((codec = av_codec_iterate(&iter))) {
        if (codec->type == AVMEDIA_TYPE_VIDEO && av_codec_is_decoder(codec)) {
            if (isImageCodec(codec->id)) {
                codecs << QString(codec->name);
            }
        }
    }

    codecs.removeDuplicates();
    codecs.sort();
    return codecs;
}