#include "AudioPlayer.h"
#include <QDebug>
#include <QThread>

// AudioBuffer implementation
AudioBuffer::AudioBuffer(QObject *parent) : QIODevice(parent), m_currentPos(0) {
    open(QIODevice::ReadWrite);
}

void AudioBuffer::writeData(const QByteArray &data) {
    QMutexLocker locker(&m_mutex);
    m_bufferQueue.push(data);
}

void AudioBuffer::clear() {
    QMutexLocker locker(&m_mutex);
    while (!m_bufferQueue.empty()) {
        m_bufferQueue.pop();
    }
    m_currentBuffer.clear();
    m_currentPos = 0;
}

bool AudioBuffer::isEmpty() const {
    QMutexLocker locker(&m_mutex);
    return m_bufferQueue.empty() && (m_currentPos >= m_currentBuffer.size());
}

qint64 AudioBuffer::readData(char *data, qint64 maxlen) {
    QMutexLocker locker(&m_mutex);

    qint64 totalRead = 0;

    while (totalRead < maxlen) {
        // If current buffer is exhausted, get next one from queue
        if (m_currentPos >= m_currentBuffer.size()) {
            if (m_bufferQueue.empty()) {
                break;  // No more data available
            }
            m_currentBuffer = m_bufferQueue.front();
            m_bufferQueue.pop();
            m_currentPos = 0;
        }

        // Calculate how much we can read from current buffer
        qint64 remainingInBuffer = m_currentBuffer.size() - m_currentPos;
        qint64 toRead = qMin(maxlen - totalRead, remainingInBuffer);

        // Copy data
        memcpy(data + totalRead, m_currentBuffer.constData() + m_currentPos, toRead);
        m_currentPos += toRead;
        totalRead += toRead;
    }

    return totalRead;
}

qint64 AudioBuffer::writeData(const char *data, qint64 len) {
    Q_UNUSED(data)
    Q_UNUSED(len)
    return 0;  // We handle writing through writeData(const QByteArray&)
}

// AudioPlayer implementation
AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent),
      m_audioSink(nullptr),
      m_audioBuffer(nullptr),
      m_swrContext(nullptr),
      m_currentTime(0.0),
      m_sampleRate(44100),
      m_channels(2),
      m_initialized(false) {
    m_audioBuffer = new AudioBuffer(this);

    m_positionTimer = new QTimer(this);
    m_positionTimer->setInterval(100);  // Update every 100ms
    connect(m_positionTimer, &QTimer::timeout, this, &AudioPlayer::updatePosition);

    // Initialize time base to default
    m_timeBase = av_make_q(1, 1000000);  // microseconds
}

AudioPlayer::~AudioPlayer() {
    stop();
    if (m_swrContext) {
        swr_free(&m_swrContext);
    }
    if (m_audioSink) {
        m_audioSink->deleteLater();
    }
}

bool AudioPlayer::initialize(AVCodecContext *audioCodecContext) {
    if (!audioCodecContext) {
        qDebug() << "AudioPlayer: Invalid audio codec context";
        return false;
    }

    // FFmpeg 4.4 compatibility - use channels instead of ch_layout
    int channels = audioCodecContext->channels;
    qDebug() << "AudioPlayer: Initializing with sample rate:" << audioCodecContext->sample_rate
             << "channels:" << channels
             << "format:" << av_get_sample_fmt_name(audioCodecContext->sample_fmt);

    m_sampleRate = audioCodecContext->sample_rate;
    m_channels = channels;  // Setup audio format for Qt
    if (!setupAudioFormat(audioCodecContext)) {
        qDebug() << "AudioPlayer: Failed to setup audio format";
        return false;
    }

    // Create audio sink
    m_audioSink = new QAudioSink(m_audioFormat, this);
    if (!m_audioSink) {
        qDebug() << "AudioPlayer: Failed to create audio sink";
        return false;
    }

    connect(m_audioSink, QOverload<QAudio::State>::of(&QAudioSink::stateChanged), this,
            &AudioPlayer::onAudioStateChanged);
    m_audioSink->setVolume(1.0);

    // Setup resampler for format conversion
    m_swrContext = swr_alloc();
    if (!m_swrContext) {
        qDebug() << "AudioPlayer: Failed to allocate resampler";
        return false;
    }

    // FFmpeg 4.4 compatibility - use legacy channel layout API
    // Configure input parameters
    swr_alloc_set_opts(m_swrContext,
                       m_channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO,  // out_ch_layout
                       AV_SAMPLE_FMT_S16,                                          // out_sample_fmt
                       m_sampleRate,                       // out_sample_rate
                       audioCodecContext->channel_layout,  // in_ch_layout
                       audioCodecContext->sample_fmt,      // in_sample_fmt
                       audioCodecContext->sample_rate,     // in_sample_rate
                       0, nullptr);                        // Initialize the resampler
    if (swr_init(m_swrContext) < 0) {
        qDebug() << "AudioPlayer: Failed to initialize resampler";
        swr_free(&m_swrContext);
        return false;
    }

    m_initialized = true;
    qDebug() << "AudioPlayer: Successfully initialized";
    return true;
}

bool AudioPlayer::setupAudioFormat(AVCodecContext *codecContext) {
    m_audioFormat.setSampleRate(codecContext->sample_rate);
    m_audioFormat.setChannelCount(qMin(codecContext->channels, 2));  // Max stereo
    m_audioFormat.setSampleFormat(QAudioFormat::Int16);

    qDebug() << "AudioPlayer: Audio format - Rate:" << m_audioFormat.sampleRate()
             << "Channels:" << m_audioFormat.channelCount()
             << "Format:" << m_audioFormat.sampleFormat();

    return true;
}

void AudioPlayer::playAudioFrame(AVFrame *frame) {
    if (!frame || !m_swrContext || !m_initialized) {
        return;
    }

    // Update timing information
    updateTimeFromFrame(frame);

    // Convert audio frame to Qt-compatible format
    QByteArray audioData = convertAudioFrame(frame);
    if (!audioData.isEmpty()) {
        m_audioBuffer->writeData(audioData);

        // Emit buffer level changed signal
        // This is a rough estimate - you might want to implement more sophisticated buffering
        emit bufferLevelChanged(50);  // Placeholder value
    }
}

QByteArray AudioPlayer::convertAudioFrame(AVFrame *frame) {
    if (!m_swrContext) {
        return QByteArray();
    }

    // Calculate output samples
    int out_samples = swr_get_out_samples(m_swrContext, frame->nb_samples);
    if (out_samples < 0) {
        qDebug() << "AudioPlayer: Error getting output samples count";
        return QByteArray();
    }

    int out_channels = m_audioFormat.channelCount();

    // Allocate output buffer
    uint8_t *out_buffer = nullptr;
    int out_linesize;
    int ret = av_samples_alloc(&out_buffer, &out_linesize, out_channels, out_samples,
                               AV_SAMPLE_FMT_S16, 0);
    if (ret < 0) {
        qDebug() << "AudioPlayer: Failed to allocate output buffer";
        return QByteArray();
    }

    // Convert audio samples
    int converted_samples = swr_convert(m_swrContext, &out_buffer, out_samples,
                                        (const uint8_t **)frame->data, frame->nb_samples);

    if (converted_samples < 0) {
        qDebug() << "AudioPlayer: Error converting audio samples";
        av_freep(&out_buffer);
        return QByteArray();
    }

    // Create QByteArray from converted data
    int data_size = converted_samples * out_channels * sizeof(int16_t);
    QByteArray result((const char *)out_buffer, data_size);

    av_freep(&out_buffer);
    return result;
}

void AudioPlayer::updateTimeFromFrame(AVFrame *frame) {
    if (frame->pts != AV_NOPTS_VALUE) {
        // Convert PTS to seconds using time base
        m_currentTime = frame->pts * av_q2d(m_timeBase);
    }
}

void AudioPlayer::start() {
    if (!m_audioSink || !m_initialized) {
        qDebug() << "AudioPlayer: Cannot start - not initialized";
        return;
    }

    m_audioSink->start(m_audioBuffer);
    m_positionTimer->start();
}

void AudioPlayer::pause() {
    if (m_audioSink) {
        qDebug() << "AudioPlayer: Pausing playback";
        m_audioSink->suspend();
        m_positionTimer->stop();
    }
}

void AudioPlayer::resume() {
    if (m_audioSink) {
        qDebug() << "AudioPlayer: Resuming playback";
        m_audioSink->resume();
        m_positionTimer->start();
    }
}

void AudioPlayer::stop() {
    if (m_audioSink) {
        qDebug() << "AudioPlayer: Stopping playback";
        m_audioSink->stop();
        m_positionTimer->stop();
    }

    if (m_audioBuffer) {
        m_audioBuffer->clear();
    }

    m_currentTime = 0.0;
}

void AudioPlayer::setVolume(qreal volume) {
    if (m_audioSink) {
        m_audioSink->setVolume(qBound(0.0, volume, 1.0));
        qDebug() << "AudioPlayer: Volume set to" << volume;
    }
}

bool AudioPlayer::isPlaying() const {
    return m_audioSink && (m_audioSink->state() == QAudio::ActiveState);
}

qreal AudioPlayer::getVolume() const { return m_audioSink ? m_audioSink->volume() : 0.0; }

bool AudioPlayer::hasBufferedData() const { return m_audioBuffer && !m_audioBuffer->isEmpty(); }

void AudioPlayer::clearBuffer() {
    if (m_audioBuffer) {
        m_audioBuffer->clear();
    }
}

void AudioPlayer::updatePosition() {
    if (m_audioSink) {
        qint64 position = m_audioSink->processedUSecs();
        emit positionChanged(position);
    }
}

void AudioPlayer::onAudioStateChanged() {
    if (!m_audioSink) return;

    QAudio::State state = m_audioSink->state();
    // qDebug() << "AudioPlayer: State changed to" << state;
    emit stateChanged(static_cast<int>(state));

    switch (state) {
    case QAudio::StoppedState:
        if (m_audioSink->error() != QAudio::NoError) {
            qDebug() << "AudioPlayer: Error occurred:" << m_audioSink->error();
        }
        break;
        // case QAudio::ActiveState: qDebug() << "AudioPlayer: Playback active"; break;
        // case QAudio::SuspendedState: qDebug() << "AudioPlayer: Playback suspended"; break;
        // case QAudio::IdleState: qDebug() << "AudioPlayer: Playback idle"; break;
    }
}

#include "AudioPlayer.moc"