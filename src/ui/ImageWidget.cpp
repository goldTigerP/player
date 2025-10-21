#include "ui/ImageWidget.h"
#include "QLabelImageWidget.h"

ImageWidget* ImageWidget::createImageWidget(QWidget* parent) {
    return new QLabelImageWidget(parent);
}