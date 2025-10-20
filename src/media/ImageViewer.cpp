#include "media/ImageViewer.h"
#include <QDebug>
#include <QFileInfo>
#include <QBuffer>
#include <QImageReader>

ImageViewer::ImageViewer(QObject* parent)
    : QObject(parent)
    , m_rotation(0.0)
    , m_scaleX(1.0)
    , m_scaleY(1.0)
    , m_flipHorizontalFlag(false)
    , m_flipVerticalFlag(false)
    , m_zoomLevel(1.0)
    , m_brightness(0)
    , m_contrast(1.0)
    , m_saturation(1.0)
{
    resetParameters();
}

ImageViewer::~ImageViewer() {
    // OpenCV Mat会自动管理内存
}

bool ImageViewer::loadImage(const QString& filePath) {
    if (!QFile::exists(filePath)) {
        emit error(QString("Image file does not exist: %1").arg(filePath));
        return false;
    }
    
    try {
        // 使用OpenCV加载图片
        m_originalMat = cv::imread(filePath.toStdString(), cv::IMREAD_COLOR);
        
        if (m_originalMat.empty()) {
            emit error(QString("Failed to load image: %1").arg(filePath));
            return false;
        }
        
        // 转换BGR到RGB
        if (m_originalMat.channels() == 3) {
            cv::cvtColor(m_originalMat, m_originalMat, cv::COLOR_BGR2RGB);
        }
        
        m_currentMat = m_originalMat.clone();
        m_processedMat = m_originalMat.clone();
        
        // 提取图片信息
        extractImageInfo(filePath);
        
        // 重置参数
        resetParameters();
        
        // 更新显示
        updatePixmap();
        
        emit imageLoaded(filePath);
        return true;
        
    } catch (const cv::Exception& e) {
        emit error(QString("OpenCV error: %1").arg(e.what()));
        return false;
    }
}

bool ImageViewer::loadImageFromData(const QByteArray& data) {
    try {
        // 将QByteArray转换为cv::Mat
        std::vector<uchar> buffer(data.begin(), data.end());
        m_originalMat = cv::imdecode(buffer, cv::IMREAD_COLOR);
        
        if (m_originalMat.empty()) {
            emit error("Failed to decode image data");
            return false;
        }
        
        // 转换BGR到RGB
        if (m_originalMat.channels() == 3) {
            cv::cvtColor(m_originalMat, m_originalMat, cv::COLOR_BGR2RGB);
        }
        
        m_currentMat = m_originalMat.clone();
        m_processedMat = m_originalMat.clone();
        
        // 设置基本信息
        m_imageInfo.width = m_originalMat.cols;
        m_imageInfo.height = m_originalMat.rows;
        m_imageInfo.channels = m_originalMat.channels();
        m_imageInfo.format = ImageFormat::Unknown;
        
        resetParameters();
        updatePixmap();
        
        emit imageLoaded("");
        return true;
        
    } catch (const cv::Exception& e) {
        emit error(QString("OpenCV error: %1").arg(e.what()));
        return false;
    }
}

void ImageViewer::rotate(double angle) {
    m_rotation += angle;
    while (m_rotation >= 360.0) m_rotation -= 360.0;
    while (m_rotation < 0.0) m_rotation += 360.0;
    
    applyTransforms();
    updatePixmap();
    emit transformChanged();
}

void ImageViewer::scale(double factor) {
    m_scaleX *= factor;
    m_scaleY *= factor;
    
    applyTransforms();
    updatePixmap();
    emit transformChanged();
}

void ImageViewer::flipHorizontal() {
    m_flipHorizontalFlag = !m_flipHorizontalFlag;
    applyTransforms();
    updatePixmap();
    emit transformChanged();
}

void ImageViewer::flipVertical() {
    m_flipVerticalFlag = !m_flipVerticalFlag;
    applyTransforms();
    updatePixmap();
    emit transformChanged();
}

void ImageViewer::resetTransform() {
    m_rotation = 0.0;
    m_scaleX = 1.0;
    m_scaleY = 1.0;
    m_flipHorizontalFlag = false;
    m_flipVerticalFlag = false;
    m_zoomLevel = 1.0;
    
    applyTransforms();
    updatePixmap();
    emit transformChanged();
}

void ImageViewer::adjustBrightness(int value) {
    m_brightness = qBound(-100, value, 100);
    applyAdjustments();
    updatePixmap();
    emit imageProcessed();
}

void ImageViewer::adjustContrast(double factor) {
    m_contrast = qBound(0.1, factor, 3.0);
    applyAdjustments();
    updatePixmap();
    emit imageProcessed();
}

void ImageViewer::adjustSaturation(double factor) {
    m_saturation = qBound(0.0, factor, 2.0);
    applyAdjustments();
    updatePixmap();
    emit imageProcessed();
}

void ImageViewer::applyGaussianBlur(int kernelSize) {
    if (m_currentMat.empty()) return;
    
    try {
        // 确保核大小为奇数
        if (kernelSize % 2 == 0) kernelSize++;
        kernelSize = qBound(3, kernelSize, 31);
        
        cv::GaussianBlur(m_currentMat, m_processedMat, cv::Size(kernelSize, kernelSize), 0);
        m_currentMat = m_processedMat.clone();
        
        updatePixmap();
        emit imageProcessed();
        
    } catch (const cv::Exception& e) {
        emit error(QString("Gaussian blur error: %1").arg(e.what()));
    }
}

void ImageViewer::applySharpen() {
    if (m_currentMat.empty()) return;
    
    try {
        // 创建锐化核
        cv::Mat kernel = (cv::Mat_<float>(3, 3) << 
            0, -1, 0,
            -1, 5, -1,
            0, -1, 0);
        
        cv::filter2D(m_currentMat, m_processedMat, -1, kernel);
        m_currentMat = m_processedMat.clone();
        
        updatePixmap();
        emit imageProcessed();
        
    } catch (const cv::Exception& e) {
        emit error(QString("Sharpen error: %1").arg(e.what()));
    }
}

void ImageViewer::convertToGrayscale() {
    if (m_currentMat.empty()) return;
    
    try {
        if (m_currentMat.channels() == 3) {
            cv::cvtColor(m_currentMat, m_processedMat, cv::COLOR_RGB2GRAY);
            cv::cvtColor(m_processedMat, m_currentMat, cv::COLOR_GRAY2RGB);
        }
        
        updatePixmap();
        emit imageProcessed();
        
    } catch (const cv::Exception& e) {
        emit error(QString("Grayscale conversion error: %1").arg(e.what()));
    }
}

void ImageViewer::resetAdjustments() {
    m_brightness = 0;
    m_contrast = 1.0;
    m_saturation = 1.0;
    
    m_currentMat = m_originalMat.clone();
    applyTransforms();
    updatePixmap();
    emit imageProcessed();
}

void ImageViewer::fitToSize(const QSize& size, Qt::AspectRatioMode mode) {
    if (m_originalMat.empty()) return;
    
    QSize imageSize(m_originalMat.cols, m_originalMat.rows);
    QSize targetSize = imageSize.scaled(size, mode);
    
    double scaleX = double(targetSize.width()) / imageSize.width();
    double scaleY = double(targetSize.height()) / imageSize.height();
    
    if (mode == Qt::KeepAspectRatio) {
        double scale = qMin(scaleX, scaleY);
        m_scaleX = scale;
        m_scaleY = scale;
    } else {
        m_scaleX = scaleX;
        m_scaleY = scaleY;
    }
    
    m_zoomLevel = m_scaleX;
    
    applyTransforms();
    updatePixmap();
    emit transformChanged();
}

void ImageViewer::zoomIn(double factor) {
    setZoomLevel(m_zoomLevel * factor);
}

void ImageViewer::zoomOut(double factor) {
    setZoomLevel(m_zoomLevel / factor);
}

void ImageViewer::setZoomLevel(double level) {
    m_zoomLevel = qBound(0.1, level, 10.0);
    m_scaleX = m_zoomLevel;
    m_scaleY = m_zoomLevel;
    
    applyTransforms();
    updatePixmap();
    emit transformChanged();
}

void ImageViewer::applyTransforms() {
    if (m_originalMat.empty()) return;
    
    try {
        cv::Mat temp = m_originalMat.clone();
        
        // 应用亮度、对比度、饱和度调整
        if (m_brightness != 0 || m_contrast != 1.0) {
            temp.convertTo(temp, -1, m_contrast, m_brightness);
        }
        
        // 应用旋转
        if (m_rotation != 0.0) {
            cv::Point2f center(temp.cols / 2.0f, temp.rows / 2.0f);
            cv::Mat rotationMatrix = cv::getRotationMatrix2D(center, m_rotation, 1.0);
            cv::warpAffine(temp, temp, rotationMatrix, temp.size());
        }
        
        // 应用翻转
        if (m_flipHorizontalFlag) {
            cv::flip(temp, temp, 1);
        }
        if (m_flipVerticalFlag) {
            cv::flip(temp, temp, 0);
        }
        
        // 应用缩放
        if (m_scaleX != 1.0 || m_scaleY != 1.0) {
            cv::Size newSize(temp.cols * m_scaleX, temp.rows * m_scaleY);
            cv::resize(temp, temp, newSize);
        }
        
        m_currentMat = temp;
        
    } catch (const cv::Exception& e) {
        emit error(QString("Transform error: %1").arg(e.what()));
        m_currentMat = m_originalMat.clone();
    }
}

void ImageViewer::applyAdjustments() {
    if (m_originalMat.empty()) return;
    
    try {
        cv::Mat temp = m_originalMat.clone();
        
        // 应用亮度和对比度
        if (m_brightness != 0 || m_contrast != 1.0) {
            temp.convertTo(temp, -1, m_contrast, m_brightness);
        }
        
        // 应用饱和度（转换到HSV色彩空间）
        if (m_saturation != 1.0) {
            cv::Mat hsv;
            cv::cvtColor(temp, hsv, cv::COLOR_RGB2HSV);
            
            std::vector<cv::Mat> channels;
            cv::split(hsv, channels);
            
            // 调整饱和度通道
            channels[1] *= m_saturation;
            cv::merge(channels, hsv);
            
            cv::cvtColor(hsv, temp, cv::COLOR_HSV2RGB);
        }
        
        m_currentMat = temp;
        
    } catch (const cv::Exception& e) {
        emit error(QString("Adjustment error: %1").arg(e.what()));
        m_currentMat = m_originalMat.clone();
    }
}

void ImageViewer::updatePixmap() {
    if (!m_currentMat.empty()) {
        m_pixmap = matToQPixmap(m_currentMat);
    } else {
        m_pixmap = QPixmap();
    }
}

QPixmap ImageViewer::matToQPixmap(const cv::Mat& mat) {
    if (mat.empty()) return QPixmap();
    
    QImage qimg;
    
    if (mat.channels() == 1) {
        // 灰度图像
        qimg = QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
    } else if (mat.channels() == 3) {
        // RGB图像
        qimg = QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
    } else if (mat.channels() == 4) {
        // RGBA图像
        qimg = QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGBA8888);
    }
    
    return QPixmap::fromImage(qimg);
}

cv::Mat ImageViewer::qPixmapToMat(const QPixmap& pixmap) {
    QImage qimg = pixmap.toImage();
    qimg = qimg.convertToFormat(QImage::Format_RGB888);
    
    return cv::Mat(qimg.height(), qimg.width(), CV_8UC3, 
                   (void*)qimg.constBits(), qimg.bytesPerLine()).clone();
}

void ImageViewer::extractImageInfo(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    
    m_imageInfo.filePath = filePath;
    m_imageInfo.format = detectFormat(filePath);
    m_imageInfo.width = m_originalMat.cols;
    m_imageInfo.height = m_originalMat.rows;
    m_imageInfo.channels = m_originalMat.channels();
    m_imageInfo.bitDepth = m_originalMat.elemSize1() * 8;
    m_imageInfo.fileSize = fileInfo.size();
}

ImageFormat ImageViewer::detectFormat(const QString& filePath) {
    QString suffix = QFileInfo(filePath).suffix().toLower();
    
    if (suffix == "jpg" || suffix == "jpeg") return ImageFormat::JPEG;
    if (suffix == "png") return ImageFormat::PNG;
    if (suffix == "bmp") return ImageFormat::BMP;
    if (suffix == "tiff" || suffix == "tif") return ImageFormat::TIFF;
    if (suffix == "gif") return ImageFormat::GIF;
    if (suffix == "webp") return ImageFormat::WEBP;
    
    return ImageFormat::Unknown;
}

void ImageViewer::resetParameters() {
    m_rotation = 0.0;
    m_scaleX = 1.0;
    m_scaleY = 1.0;
    m_flipHorizontalFlag = false;
    m_flipVerticalFlag = false;
    m_zoomLevel = 1.0;
    m_brightness = 0;
    m_contrast = 1.0;
    m_saturation = 1.0;
}

QStringList ImageViewer::getSupportedFormats() {
    return QStringList() << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" 
                        << "*.tiff" << "*.tif" << "*.gif" << "*.webp";
}

bool ImageViewer::isFormatSupported(const QString& filePath) {
    QString suffix = QFileInfo(filePath).suffix().toLower();
    QStringList supported = getSupportedFormats();
    
    for (const QString& format : supported) {
        if (format.mid(2) == suffix) { // 去掉"*."前缀
            return true;
        }
    }
    
    return false;
}