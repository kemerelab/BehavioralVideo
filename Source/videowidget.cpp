#include "videowidget.h"
#include <QDebug>

VideoWidget::VideoWidget(QWidget *parent) :
    QWidget(parent)
{
    setAutoFillBackground(false);
    //setAttribute(Qt::WA_NoSystemBackground, true);
    //setAttribute(Qt::WA_PaintOnScreen, true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setPalette(palette);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    //currentFrame = QImage(640,480,QImage::Format_RGB888);
}

void VideoWidget::newFrame(const QImage &image) {
    qDebug() << "Slot triggered.";
    currentFrame = image;
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    //Set the painter to use a smooth scaling algorithm.
    p.setRenderHint(QPainter::SmoothPixmapTransform, 1);

    p.drawImage(this->rect(), currentFrame);
}
