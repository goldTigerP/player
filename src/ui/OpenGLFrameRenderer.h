#pragma once

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>

extern "C" {
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

class OpenGLFrameRenderer : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    explicit OpenGLFrameRenderer(QWidget *parent = nullptr);
    ~OpenGLFrameRenderer() override;

    // 渲染AVFrame
    void renderFrame(AVFrame *frame);

    // 清空显示
    void clearFrame();

    // 设置显示模式
    void setAspectRatioMode(Qt::AspectRatioMode mode);
    void setZoom(float factor);

    // 获取信息
    bool hasFrame() const { return m_hasFrame; }
    QSize frameSize() const { return m_frameSize; }

signals:
    void glReady();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

private:
    // 初始化着色器和缓冲区
    void setupYUVShader();
    void setupRGBShader();
    void setupBuffers();

    // 更新纹理
    void updateYUVTextures(AVFrame *frame);
    void updateRGBTexture(AVFrame *frame);

    // 计算变换矩阵
    void calculateTransform();

    // 清理资源
    void cleanupGL();

    // 着色器程序
    QOpenGLShaderProgram *m_yuvShader;
    QOpenGLShaderProgram *m_rgbShader;
    QOpenGLShaderProgram *m_currentShader;

    // OpenGL缓冲区
    GLuint m_VAO;
    QOpenGLBuffer m_vertexBuffer;

    // YUV纹理
    GLuint m_textureY;
    GLuint m_textureU;
    GLuint m_textureV;

    // RGB纹理
    GLuint m_textureRGB;

    // 变换矩阵
    QMatrix4x4 m_modelMatrix;

    // 显示参数
    Qt::AspectRatioMode m_aspectRatioMode;
    float m_zoomFactor;
    float m_rotation;

    // 帧信息
    bool m_hasFrame;
    QSize m_frameSize;
    int m_frameFormat;  // AVPixelFormat

    // 格式转换上下文（如果需要）
    SwsContext *m_swsContext;
    AVFrame *m_convertedFrame;
};
