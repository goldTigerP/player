#pragma once

#include "QLabel"
#include "ui/ImageWidget.h"

class QLabelImageWidget : public ImageWidget {
public:
    explicit QLabelImageWidget(QWidget *parent) : ImageWidget(parent) {
        m_label = new QLabel(this);
    }

    void loadImage(const QString &filePath) override {
        QPixmap pixmap(filePath);
        m_label->setPixmap(pixmap);
        m_label->setScaledContents(true);
        m_label->show();
    }

private:
    QLabel *m_label;
};