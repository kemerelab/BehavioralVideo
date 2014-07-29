#ifndef PTGREYINTERFACE_H
#define PTGREYINTERFACE_H

#include <QObject>
#include "ffmpeg.h"
#include <FlyCapture2.h>
#include <QImage>
#include "videowriter.h"


void OnImageGrabbed(FlyCapture2::Image* pImage, const void* pCallbackData);

struct ImageWithMetadata {
    FlyCapture2::Image flyCapImage;
    unsigned int flag;
};

//Q_DECLARE_METATYPE(FlyCapture2::Image);
Q_DECLARE_METATYPE(ImageWithMetadata);

class PtGreyInterface : public QObject
{
    Q_OBJECT
public:
    explicit PtGreyInterface(QObject *parent = 0);
     ~PtGreyInterface(void);
    bool isochStarted;


signals:
    void newFrame(QImage frame);
    void capturingStarted(void);
    void capturingEnded(void);
    void initializeVideo(QString filename);


public slots:
    void Initialize(uint serialnumber);
    //void FrameReceived(void);
    //void FrameReceived(FlyCapture2::Image pImage);
    void FrameReceived(ImageWithMetadata img);
    void StartCapture(bool enableStrobe);
    void StartCaptureNoStrobe(void);
    void StartCaptureWithStrobe(void);
    void StopCapture(void);


public:
    VideoWriter* videowriter;
private:
    int width, height;
    FlyCapture2::PixelFormat pixFmt;
    QImage *currentFrame;
    FlyCapture2::Camera cam;
    FlyCapture2::Image *pImage; //PtGrey format
    FlyCapture2::VideoMode videoMode;
    FlyCapture2::FrameRate frameRate;
    FlyCapture2::Property shutter;
    FlyCapture2::Property gain;


    bool isCapturing;
    bool strobeEnabled;
    PixelFormat vPixFmt;

    FlyCapture2::StrobeControl strobeControl, edgeStrobeControl;
    FlyCapture2::TriggerMode triggerMode;
    int lastGPIOPinState;

    AVFrame *currentFrame_RAW;
    void *currentFrame_RAW_buf;
    AVFrame *currentFrame_RGB;
    struct SwsContext *sws_ctx;


signals:

public slots:

};

#endif // PTGREYINTERFACE_H
