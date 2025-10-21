#pragma once

#include <QWidget>

class ImageWidget : public QWidget {
public:
    static ImageWidget *createImageWidget(QWidget *parent = nullptr);

    explicit ImageWidget(QWidget *parent) : QWidget(parent) {}
    virtual void loadImage(const QString &filePath) = 0;
};