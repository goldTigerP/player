#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtGui/QPixmap>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtCore/QTimer>
#include <memory>

#include "core/MediaManager.h"

class MediaDisplayWidget : public QWidget {
    Q_OBJECT

public:
    explicit MediaDisplayWidget(QWidget* parent = nullptr);
    ~MediaDisplayWidget();

    // 显示控制
    void setMediaManager(std::shared_ptr<MediaManager> manager);
    void clear();
    void updateDisplay();
    
    // 缩放控制
    void zoomIn(double factor = 1.2);
    void zoomOut(double factor = 0.8);
    void fitToWindow();
    void actualSize();
    void setZoomLevel(double level);
    double getZoomLevel() const { return m_zoomLevel; }
    
    // 显示模式
    enum class DisplayMode {
        FitToWindow,
        ActualSize,
        Custom
    };
    
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const { return m_displayMode; }
    
    // 背景设置
    void setBackgroundColor(const QColor& color);
    QColor getBackgroundColor() const { return m_backgroundColor; }

signals:
    void clicked();
    void doubleClicked();
    void zoomChanged(double level);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void onFrameReady(const QByteArray& frameData, int width, int height);
    void onImageLoaded(const QString& filePath);
    void onMediaTypeChanged(MediaFileType type);
    void updateFrame();

private:
    // 媒体管理器
    std::shared_ptr<MediaManager> m_mediaManager;
    
    // 显示内容
    QPixmap m_currentPixmap;
    QSize m_originalSize;
    QRect m_displayRect;
    
    // 显示参数
    DisplayMode m_displayMode;
    double m_zoomLevel;
    QColor m_backgroundColor;
    
    // 拖拽支持
    bool m_dragging;
    QPoint m_lastPanPoint;
    QPoint m_panOffset;
    
    // 媒体类型
    MediaFileType m_currentMediaType;
    
    // 更新定时器
    QTimer* m_updateTimer;
    
    // 私有方法
    void calculateDisplayRect();
    void drawContent(QPainter& painter);
    void drawVideoFrame(QPainter& painter);
    void drawImage(QPainter& painter);
    void drawPlaceholder(QPainter& painter);
    
    QRect getScaledRect(const QSize& imageSize, const QRect& targetRect, Qt::AspectRatioMode mode) const;
    void updateZoom();
    void resetPan();
};