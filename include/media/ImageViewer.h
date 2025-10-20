#pragma once

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <opencv2/opencv.hpp>

enum class ImageFormat {
    Unknown,
    JPEG,
    PNG,
    BMP,
    TIFF,
    GIF,
    WEBP
};

struct ImageInfo {
    QString filePath;
    ImageFormat format;
    int width;
    int height;
    int channels;
    int bitDepth;
    qint64 fileSize;
};

class ImageViewer : public QObject {
    Q_OBJECT

public:
    explicit ImageViewer(QObject* parent = nullptr);
    ~ImageViewer();

    // 图片加载
    bool loadImage(const QString& filePath);
    bool loadImageFromData(const QByteArray& data);
    
    // 图片信息
    ImageInfo getImageInfo() const { return m_imageInfo; }
    QPixmap getPixmap() const { return m_pixmap; }
    cv::Mat getCurrentMat() const { return m_currentMat; }
    
    // 图片变换
    void rotate(double angle);
    void scale(double factor);
    void flipHorizontal();
    void flipVertical();
    void resetTransform();
    
    // 图片处理
    void adjustBrightness(int value); // -100 to 100
    void adjustContrast(double factor); // 0.1 to 3.0
    void adjustSaturation(double factor); // 0.0 to 2.0
    void applyGaussianBlur(int kernelSize);
    void applySharpen();
    void convertToGrayscale();
    void resetAdjustments();
    
    // 缩放和适配
    void fitToSize(const QSize& size, Qt::AspectRatioMode mode = Qt::KeepAspectRatio);
    void zoomIn(double factor = 1.2);
    void zoomOut(double factor = 0.8);
    void setZoomLevel(double level);
    double getZoomLevel() const { return m_zoomLevel; }
    
    // 支持的格式
    static QStringList getSupportedFormats();
    static bool isFormatSupported(const QString& filePath);

signals:
    void imageLoaded(const QString& filePath);
    void imageProcessed();
    void transformChanged();
    void error(const QString& errorString);

private:
    // OpenCV相关
    cv::Mat m_originalMat;
    cv::Mat m_currentMat;
    cv::Mat m_processedMat;
    
    // Qt相关
    QPixmap m_pixmap;
    
    // 图片信息
    ImageInfo m_imageInfo;
    
    // 变换参数
    double m_rotation;
    double m_scaleX;
    double m_scaleY;
    bool m_flipHorizontalFlag;
    bool m_flipVerticalFlag;
    double m_zoomLevel;
    
    // 调整参数
    int m_brightness;
    double m_contrast;
    double m_saturation;
    
    // 私有方法
    void updatePixmap();
    void applyTransforms();
    void applyAdjustments();
    QPixmap matToQPixmap(const cv::Mat& mat);
    cv::Mat qPixmapToMat(const QPixmap& pixmap);
    ImageFormat detectFormat(const QString& filePath);
    void extractImageInfo(const QString& filePath);
    void resetParameters();
};