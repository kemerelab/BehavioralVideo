#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QPainter>
//#include <QGLWidget>


class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = 0);
    //QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *event);
    //void resizeEvent(QResizeEvent *event);

signals:

public slots:
    void newFrame(const QImage &image);

private:
    QImage currentFrame;
};

#endif // VIDEOWIDGET_H
