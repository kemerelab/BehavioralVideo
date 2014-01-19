#include "videoglwidget.h"

#include <QtCore/QCoreApplication>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>

#include <QDebug>

VideoGLWidget::VideoGLWidget()
{
    //currentFrame = ;
    v_width = -1;
    v_height = -1;
}

void VideoGLWidget::initialize() {
    glGenTextures(1,&m_texture);
    glBindTexture(GL_TEXTURE_2D,m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    //glBindTexture(GL_TEXTURE_2D,m_texture);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, currentFrame->width(),
    //             currentFrame->height(), 0, GL_RGB, GL_UNSIGNED_BYTE, currentFrame->bits());
}

void VideoGLWidget::render()
{

    glViewport(0, 0, width(), height());

    glClearColor(0.4f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    if ((v_height != currentFrame.height()) || (v_width != currentFrame.width())) {
        v_width = currentFrame.width();
        v_height = currentFrame.height();
        glBindTexture(GL_TEXTURE_2D,m_texture);
        // allocate memory if the image size has changed (or the first time through)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, v_width,
                     v_height, 0, GL_RGB, GL_UNSIGNED_BYTE, currentFrame.bits());
    }
    else {
        glActiveTexture(m_texture);
        glBindTexture(GL_TEXTURE_2D,m_texture);
        // don't reallocate memory if the image size hasn't changed
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, v_width,
                     v_height, GL_RGB, GL_UNSIGNED_BYTE, currentFrame.bits());
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-1, -1, -1);
        glTexCoord2f(1,0); glVertex3f(1, -1, -1);
        glTexCoord2f(1,1); glVertex3f(1, 1, -1);
        glTexCoord2f(0,1); glVertex3f(-1, 1, -1);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // http://stackoverflow.com/questions/20245865/render-qimage-with-opengl
}

void VideoGLWidget::newFrame(QImage frame) {
    currentFrame = frame;
}

OpenGLWindow::OpenGLWindow(QWindow *parent) :
    QWindow(parent)
  , m_update_pending(false)
  , m_animating(false)
  , m_context(0)
  , m_device(0)
{
    setSurfaceType(QWindow::OpenGLSurface);
}


OpenGLWindow::~OpenGLWindow()
{
    delete m_device;
}

void OpenGLWindow::render(QPainter *painter)
{
    Q_UNUSED(painter);
}

void OpenGLWindow::initialize()
{
}

void OpenGLWindow::render()
{
    if (!m_device)
        m_device = new QOpenGLPaintDevice;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    m_device->setSize(size());

    QPainter painter(m_device);
    render(&painter);
}

void OpenGLWindow::renderLater()
{
    if (!m_update_pending) {
        m_update_pending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool OpenGLWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        m_update_pending = false;
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void OpenGLWindow::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);

    if (isExposed())
        renderNow();
}

void OpenGLWindow::renderNow()
{
    if (!isExposed())
        return;

    bool needsInitialize = false;

    if (!m_context) {
        m_context = new QOpenGLContext(this);
        m_context->setFormat(requestedFormat());
        m_context->create();

        needsInitialize = true;
    }

    m_context->makeCurrent(this);

    if (needsInitialize) {
        initializeOpenGLFunctions();
        initialize();
    }

    render();

    m_context->swapBuffers(this);

    if (m_animating)
        renderLater();
}

void OpenGLWindow::setAnimating(bool animating)
{
    m_animating = animating;

    if (animating)
        renderLater();
}


