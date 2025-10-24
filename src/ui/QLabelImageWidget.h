#pragma once

#include "QLabel"
#include "ui/ImageWidget.h"
#include <QEnterEvent>

class QLabelImageWidget : public ImageWidget {
public:
    explicit QLabelImageWidget(QWidget *parent) : ImageWidget(parent) {
        m_label = new QLabel(this);
    }

    void loadImage(const QString &filePath) override {
        m_pixmap = new QPixmap(filePath);
        m_label->setPixmap(*m_pixmap);
        m_label->setScaledContents(true);
        m_label->show();
        m_picRatio = (1.0 * m_pixmap->height()) / m_pixmap->width();
    }

    void resizeEvent(QResizeEvent *event) override {
        QPoint pos{0, 0};
        QSize size{event->size().width(), event->size().height()};
        if (m_pixmap != nullptr && m_picRatio != 0) {
            auto ratio = (1.0 * event->size().height()) / event->size().width();
            if (ratio > m_picRatio) {
                size.setHeight(size.width() * m_picRatio);
                pos.setY((event->size().height() - size.height()) / 2);
            } else {
                size.setWidth(size.height() / m_picRatio);
                pos.setX((event->size().width() - size.width()) / 2);
            }
        }
        m_label->move(pos);
        m_label->resize(size);
        ImageWidget::resizeEvent(event);
    }

private:
    QLabel *m_label{nullptr};
    QPixmap *m_pixmap{nullptr};
    double m_picRatio{0};
};