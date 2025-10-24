#include "OpenGLVideoWidget.h"

void OpenGLVideoWidget::showPreview() {
    connect(&m_render, &OpenGLFrameRenderer::glReady, [this]() {
        auto preview = m_videoStream.getPreviewImage();
        m_render.renderFrame(preview);
    });
    m_render.show();
}

void OpenGLVideoWidget::resizeEvent(QResizeEvent *event) {
    m_render.move({0, 0});
    m_render.resize(event->size().width(), event->size().height());
    VideoWidget::resizeEvent(event);
}
