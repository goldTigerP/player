#include "OpenGLFrameRenderer.h"
#include <QDebug>
#include <qopenglext.h>

// 🎨 YUV到RGB的顶点着色器
static const char *yuvVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;

void main() {
    gl_Position = model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

// 🎨 YUV到RGB的片段着色器 (BT.709标准)
static const char *yuvFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D textureY;
uniform sampler2D textureU;
uniform sampler2D textureV;

void main() {
    float y = texture(textureY, TexCoord).r;
    float u = texture(textureU, TexCoord).r - 0.5;
    float v = texture(textureV, TexCoord).r - 0.5;
    
    // BT.709 YUV到RGB转换
    float r = y + 1.5748 * v;
    float g = y - 0.1873 * u - 0.4681 * v;
    float b = y + 1.8556 * u;
    
    FragColor = vec4(r, g, b, 1.0);
}
)";

// 🎨 RGB顶点着色器
static const char *rgbVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;

void main() {
    gl_Position = model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

// 🎨 RGB片段着色器
static const char *rgbFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D textureRGB;

void main() {
    FragColor = texture(textureRGB, TexCoord);
}
)";

OpenGLFrameRenderer::OpenGLFrameRenderer(QWidget *parent)
    : QOpenGLWidget(parent),
      m_yuvShader(nullptr),
      m_rgbShader(nullptr),
      m_currentShader(nullptr),
      m_VAO(0),
      m_textureY(0),
      m_textureU(0),
      m_textureV(0),
      m_textureRGB(0),
      m_aspectRatioMode(Qt::KeepAspectRatio),
      m_zoomFactor(1.0f),
      m_rotation(0.0f),
      m_hasFrame(false),
      m_frameFormat(AV_PIX_FMT_NONE),
      m_swsContext(nullptr),
      m_convertedFrame(nullptr) {
    // 设置OpenGL格式
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);  // 抗锯齿
    setFormat(format);

    qDebug() << "OpenGLFrameRenderer创建";
}

OpenGLFrameRenderer::~OpenGLFrameRenderer() {
    makeCurrent();
    cleanupGL();

    if (m_swsContext) {
        sws_freeContext(m_swsContext);
    }

    if (m_convertedFrame) {
        av_frame_free(&m_convertedFrame);
    }

    doneCurrent();
}

void OpenGLFrameRenderer::initializeGL() {
    qDebug() << __FILE__ << ":" << __LINE__;
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 设置着色器
    setupYUVShader();
    setupRGBShader();
    setupBuffers();

    qDebug() << "OpenGL初始化完成";
    qDebug() << "OpenGL版本:" << (char *)glGetString(GL_VERSION);

    qDebug() << __FILE__ << __FUNCTION__ << __LINE__;
    glGenTextures(1, &m_textureY);
    qDebug() << __FILE__ << __FUNCTION__ << __LINE__;
    glGenTextures(1, &m_textureU);
    glGenTextures(1, &m_textureV);
    glGenTextures(1, &m_textureRGB);
    qDebug() << __FILE__ << __FUNCTION__ << __LINE__;

    emit glReady();
}

void OpenGLFrameRenderer::setupYUVShader() {
    m_yuvShader = new QOpenGLShaderProgram(this);

    if (!m_yuvShader->addShaderFromSourceCode(QOpenGLShader::Vertex, yuvVertexShaderSource)) {
        qDebug() << "YUV顶点着色器编译失败:" << m_yuvShader->log();
        return;
    }

    if (!m_yuvShader->addShaderFromSourceCode(QOpenGLShader::Fragment, yuvFragmentShaderSource)) {
        qDebug() << "YUV片段着色器编译失败:" << m_yuvShader->log();
        return;
    }

    if (!m_yuvShader->link()) {
        qDebug() << "YUV着色器链接失败:" << m_yuvShader->log();
        return;
    }

    qDebug() << "YUV着色器创建成功";
}

void OpenGLFrameRenderer::setupRGBShader() {
    m_rgbShader = new QOpenGLShaderProgram(this);

    if (!m_rgbShader->addShaderFromSourceCode(QOpenGLShader::Vertex, rgbVertexShaderSource)) {
        qDebug() << "RGB顶点着色器编译失败:" << m_rgbShader->log();
        return;
    }

    if (!m_rgbShader->addShaderFromSourceCode(QOpenGLShader::Fragment, rgbFragmentShaderSource)) {
        qDebug() << "RGB片段着色器编译失败:" << m_rgbShader->log();
        return;
    }

    if (!m_rgbShader->link()) {
        qDebug() << "RGB着色器链接失败:" << m_rgbShader->log();
        return;
    }

    qDebug() << "RGB着色器创建成功";
}

void OpenGLFrameRenderer::setupBuffers() {
    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  // 左下
        1.0f,  -1.0f, 0.0f, 1.0f, 1.0f,  // 右下
        1.0f,  1.0f,  0.0f, 1.0f, 0.0f,  // 右上
        -1.0f, 1.0f,  0.0f, 0.0f, 0.0f   // 左上
    };

    // clang-format off
    // 定义两个三角形
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    // clang-format on

    // 创建VAO
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    // 顶点缓冲
    m_vertexBuffer.create();
    m_vertexBuffer.bind();
    m_vertexBuffer.allocate(vertices, sizeof(vertices));

    // 索引缓冲
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 顶点属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void OpenGLFrameRenderer::renderFrame(AVFrame *frame) {
    qDebug() << "render begin render frame";

    if (!frame || !frame->data[0]) {
        qDebug() << "无效的帧";
        return;
    }

    makeCurrent();

    // 更新帧信息
    m_hasFrame = true;
    m_frameSize = QSize(frame->width, frame->height);
    m_frameFormat = frame->format;

    // 根据帧格式选择处理方式
    switch (frame->format) {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
        qDebug() << "render AV_PIX_FMT_YUVJ420P";
        m_currentShader = m_yuvShader;
        updateYUVTextures(frame);
        break;

    case AV_PIX_FMT_RGB24:
    case AV_PIX_FMT_RGBA:
    case AV_PIX_FMT_BGRA:
        qDebug() << "render AV_PIX_FMT_BGRA";
        m_currentShader = m_rgbShader;
        updateRGBTexture(frame);
        break;

    default:
        // 转换为YUV420P
        // qDebug() << "不支持的格式，转换为YUV420P:"
        //          << av_get_pix_fmt_name((AVPixelFormat)frame->format);
        // TODO: 实现格式转换
        break;
    }

    doneCurrent();
    update();  // 触发重绘
}

void OpenGLFrameRenderer::updateYUVTextures(AVFrame *frame) {
    qDebug() << "render updateYUVTextures";

    qDebug() << __FILE__ << __FUNCTION__ << __LINE__;
    glBindTexture(GL_TEXTURE_2D, m_textureY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width, frame->height, 0, GL_RED, GL_UNSIGNED_BYTE,
                 frame->data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    qDebug() << __FILE__ << __FUNCTION__ << __LINE__;
    int chromaWidth = frame->width / 2;
    int chromaHeight = frame->height / 2;

    qDebug() << __FILE__ << __FUNCTION__ << __LINE__;
    glBindTexture(GL_TEXTURE_2D, m_textureU);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, chromaWidth, chromaHeight, 0, GL_RED, GL_UNSIGNED_BYTE,
                 frame->data[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    qDebug() << __FILE__ << __FUNCTION__ << __LINE__;
    glBindTexture(GL_TEXTURE_2D, m_textureV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, chromaWidth, chromaHeight, 0, GL_RED, GL_UNSIGNED_BYTE,
                 frame->data[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    qDebug() << __FILE__ << __FUNCTION__ << __LINE__;
    glBindTexture(GL_TEXTURE_2D, 0);
    qDebug() << __FILE__ << __FUNCTION__ << __LINE__;
}

void OpenGLFrameRenderer::updateRGBTexture(AVFrame *frame) {
    if (m_textureRGB == 0) {
        glGenTextures(1, &m_textureRGB);
    }

    glBindTexture(GL_TEXTURE_2D, m_textureRGB);

    GLenum format = (frame->format == AV_PIX_FMT_RGB24) ? GL_RGB : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, frame->width, frame->height, 0, format, GL_UNSIGNED_BYTE,
                 frame->data[0]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLFrameRenderer::paintGL() {
    qDebug() << "render paintGL";

    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_hasFrame || !m_currentShader) {
        return;
    }

    m_currentShader->bind();

    // 绑定纹理
    if (m_currentShader == m_yuvShader) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureY);
        m_currentShader->setUniformValue("textureY", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_textureU);
        m_currentShader->setUniformValue("textureU", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_textureV);
        m_currentShader->setUniformValue("textureV", 2);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureRGB);
        m_currentShader->setUniformValue("textureRGB", 0);
    }

    // 计算并设置变换矩阵
    calculateTransform();
    m_currentShader->setUniformValue("model", m_modelMatrix);

    // 渲染
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    m_currentShader->release();
}

void OpenGLFrameRenderer::calculateTransform() {
    m_modelMatrix.setToIdentity();
    m_modelMatrix.rotate(m_rotation, 0.0f, 0.0f, 1.0f);
    m_modelMatrix.scale(m_zoomFactor);

    // 保持宽高比
    if (m_hasFrame && m_aspectRatioMode == Qt::KeepAspectRatio) {
        float frameAspect = float(m_frameSize.width()) / m_frameSize.height();
        float windowAspect = float(width()) / height();

        if (windowAspect > frameAspect) {
            m_modelMatrix.scale(frameAspect / windowAspect, 1.0f, 1.0f);
        } else {
            m_modelMatrix.scale(1.0f, windowAspect / frameAspect, 1.0f);
        }
    }
}

void OpenGLFrameRenderer::resizeGL(int w, int h) { glViewport(0, 0, w, h); }

void OpenGLFrameRenderer::clearFrame() {
    m_hasFrame = false;
    update();
}

void OpenGLFrameRenderer::setAspectRatioMode(Qt::AspectRatioMode mode) {
    m_aspectRatioMode = mode;
    update();
}

void OpenGLFrameRenderer::setZoom(float factor) {
    m_zoomFactor = qBound(0.1f, factor, 10.0f);
    update();
}

// void OpenGLFrameRenderer::setRotation(float angle) {
//     m_rotation = angle;
//     while (m_rotation >= 360.0f) m_rotation -= 360.0f;
//     while (m_rotation < 0.0f) m_rotation += 360.0f;
//     update();
// }

void OpenGLFrameRenderer::cleanupGL() {
    if (m_textureY) glDeleteTextures(1, &m_textureY);
    if (m_textureU) glDeleteTextures(1, &m_textureU);
    if (m_textureV) glDeleteTextures(1, &m_textureV);
    if (m_textureRGB) glDeleteTextures(1, &m_textureRGB);
    // if (m_VAO) glDeleteVertexArrays(1, &m_VAO);

    delete m_yuvShader;
    delete m_rgbShader;
}
