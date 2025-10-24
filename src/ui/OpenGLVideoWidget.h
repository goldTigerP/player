#pragma once

#include "OpenGLFrameRenderer.h"
#include "ui/VideoWidget.h"
#include <QEnterEvent>

class OpenGLVideoWidget : public VideoWidget {
    Q_OBJECT

public:
    explicit OpenGLVideoWidget(QWidget *parent = nullptr) : VideoWidget(parent), m_render(this) {}

    void showPreview() override;

    void resizeEvent(QResizeEvent *event) override;

private:
    OpenGLFrameRenderer m_render{};
};