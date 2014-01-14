#include "camerainterface.h"
#include <QDebug>

CameraInterface::CameraInterface(QObject *parent) :
    QObject(parent)
{
    width = 640;
    height = 480;
    //currentFrame = QVideoFrame(width * height * 3, QSize(width,height), width, QVideoFrame::Format_YUV420P);
    currentFrame = new QImage(width, height, QImage::Format_RGB888);
    if (!currentFrame)
        qDebug() << "QImage not allocated";
    currentFrame_YUV = avcodec_alloc_frame();
    if (!currentFrame_YUV)
        qDebug() << "Could not allocate frame";
    avcodec_get_frame_defaults(currentFrame_YUV);
    if(avpicture_alloc((AVPicture*)currentFrame_YUV, PIX_FMT_YUV420P,width,height)<0)
    {
        qDebug() << "Failed allocating frame in avpicture_alloc()";
    }
    frameIdx = 0;
    sws_ctx = sws_getContext(width, height, PIX_FMT_YUV420P, width, height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
    currentFrame_RGB = avcodec_alloc_frame();
    if (!currentFrame_YUV)
        qDebug() << "Could not allocate frame";
    avpicture_fill((AVPicture*)currentFrame_RGB, currentFrame->bits(), PIX_FMT_RGB24, width, height);
}

CameraInterface::~CameraInterface(void) {
    av_free(currentFrame_YUV->data[0]);
    av_free(currentFrame_YUV);
    av_free(currentFrame_RGB);
}

void CameraInterface::GenerateNextFrame(void) {
    //currentFrame.map(QAbstractVideoBuffer::write);

    int x,y;
    int i = frameIdx;
    // Y
    for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {
            currentFrame_YUV->data[0][y * width + x] = x + y + i * 3;
        }
    }

    // Cb and Cr
    for(y=0;y<height/2;y++) {
        for(x=0;x<width/2;x++) {
            currentFrame_YUV->data[1][width*height + y * width/2 + x] = 128 + y + i * 2;
            currentFrame_YUV->data[2][width*height*2 + y * width/2 + x] = 64 + x + i * 5;
        }
    }

    qDebug() << "Doing color conversion";
    sws_scale(sws_ctx, currentFrame_YUV->data, currentFrame_YUV->linesize, 0, height,
              currentFrame_RGB->data, currentFrame_RGB->linesize);
    //emit(*currentFrame);
    frameIdx++;
}
