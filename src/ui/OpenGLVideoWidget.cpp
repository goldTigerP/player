#include "OpenGLVideoWidget.h"
#include "AudioPlayer.h"

void OpenGLVideoWidget::showPreview() {
    connect(&m_render, &OpenGLFrameRenderer::glReady,
            [this]() { m_render.renderFrame(m_videoStream.getPreviewImage()); });
    m_render.show();
}

void OpenGLVideoWidget::resizeEvent(QResizeEvent *event) {
    m_render.move({0, 0});
    m_render.resize(event->size().width(), event->size().height());
    VideoWidget::resizeEvent(event);
}

void OpenGLVideoWidget::updateFrame() {
    // 只处理视频帧渲染
    double videoPts = 0.0;
    AVFrame *videoFrame = m_videoStream.getNextVideoFrame(&videoPts);
    if (videoFrame) {
        m_render.renderFrame(videoFrame);
        m_currentTime = videoPts;

        // 通知父组件时间更新（用于音视频同步）
        emit timeUpdated(videoPts);
    }
}