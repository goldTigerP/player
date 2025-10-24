#include "ui/VideoWidget.h"
#include "OpenGLVideoWidget.h"

VideoWidget *VideoWidget::createVideoWidget(QWidget *parent) {
    return new OpenGLVideoWidget(parent);
}

void VideoWidget::loadVideo(const QString &filePath) {
    m_videoStream.loadVideo(filePath);
    auto fps = m_videoStream.getFps();
    if (fps > 0) {
        m_playTimer.setInterval(1000 / fps);
    }
    showPreview();
}
