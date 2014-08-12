#ifndef VIDEOGLWIDGET_H
#define VIDEOGLWIDGET_H

#include <QObject>
#include <QtGui/QWindow>
#include <QtGui/QOpenGLFunctions>
#include <QOpenGLPaintDevice>



class OpenGLWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWindow(QWindow *parent = 0);
    ~OpenGLWindow();

    virtual void render(QPainter *painter);
    virtual void render();

    virtual void initialize();

    void setAnimating(bool animating);
    qreal aspectRatio();

signals:

public slots:
    void renderLater();
    void renderNow();

protected:
    bool event(QEvent *event);

    void exposeEvent(QExposeEvent *event);

private:
    bool m_update_pending;
    bool m_animating;

    QOpenGLContext *m_context;
    QOpenGLPaintDevice *m_device;
};

class VideoGLWidget : public OpenGLWindow
{
    Q_OBJECT
public:
    VideoGLWidget();

    void initialize();
    void render();

public slots:
    void newFrame(QImage frame);

private:
//    GLuint loadShader(GLenum type, const char *source);

    GLuint m_texture;
    GLuint m_posAttr;
    GLuint m_colAttr;
    GLuint m_matrixUniform;

//    QOpenGLShaderProgram *m_program;
    //int m_frame;
    QImage currentFrame;
    int v_width, v_height;
};

#endif // VIDEOGLWIDGET_H
