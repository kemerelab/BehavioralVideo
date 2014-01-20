#ifndef PTGREYINTERFACE_H
#define PTGREYINTERFACE_H

#include <QObject>
#include "ffmpeg.h"
#include <FlyCapture2.h>
#include <QImage>


void OnImageGrabbed(FlyCapture2::Image* pImage, const void* pCallbackData);

Q_DECLARE_METATYPE(FlyCapture2::Image);

class PtGreyInterface : public QObject
{
    Q_OBJECT
public:
    explicit PtGreyInterface(QObject *parent = 0);
     ~PtGreyInterface(void);

signals:
    void newFrame(QImage frame);

public slots:
    void Initialize(void);
    //void FrameReceived(void);
    void FrameReceived(FlyCapture2::Image pImage);
    void StartCapture(void);
    void StopCapture(void);

public:

private:
    int width, height;
    FlyCapture2::PixelFormat pixFmt;
    QImage *currentFrame;
    FlyCapture2::Camera cam;
    FlyCapture2::Image *pImage; //PtGrey format
    FlyCapture2::VideoMode videoMode;
    FlyCapture2::FrameRate frameRate;
    bool isCapturing;
    PixelFormat vPixFmt;

    AVFrame *currentFrame_RAW;
    void *currentFrame_RAW_buf;
    AVFrame *currentFrame_RGB;
    struct SwsContext *sws_ctx;
signals:

public slots:

};

#endif // PTGREYINTERFACE_H
