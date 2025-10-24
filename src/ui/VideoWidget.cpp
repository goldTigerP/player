#include "ui/VideoWidget.h"
#include "OpenGLVideoWidget.h"

VideoWidget *VideoWidget::createVideoWidget(QWidget *parent) {
    return new OpenGLVideoWidget(parent);
}

void VideoWidget::loadVideo(const QString &filePath) {
    m_videoStream.loadVideo(filePath);
    showPreview();
}
