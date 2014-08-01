#ifndef PTGREYINTERFACE_H
#define PTGREYINTERFACE_H

#include <QObject>
#include "ffmpeg.h"
#include <FlyCapture2.h>
#include <QImage>
#include "videowriter.h"


void OnImageGrabbed(FlyCapture2::Image* pImage, const void* pCallbackData);

Q_DECLARE_METATYPE(FlyCapture2::Image);

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
    void initializeVideoWriting(QString filename);


public slots:
    void Initialize(uint serialnumber);
    void InitializeVideoWriting(QString filename);
    //void FrameReceived(void);
    void FrameReceived(FlyCapture2::Image pImage);
    void StartCapture(bool enableStrobe);
    void StartCameraCaptureSync(void);
    void StartCameraCaptureAsync(void);
    void StopCapture(void);
    void StopAndRestartCaptureSync(void);
    void StopAndRestartCaptureAsync(void);
    void ChangeTriggerPin(int);


public:
    VideoWriter* videowriter;
    int serialNumber;
    bool initialized;


private:
    int width, height;
    FlyCapture2::PixelFormat pixFmt;
    QImage *currentFrame;

    FlyCapture2::Image *pImage; //PtGrey format
    FlyCapture2::VideoMode videoMode;
    FlyCapture2::FrameRate frameRate;
    FlyCapture2::Property shutter;
    FlyCapture2::Property gain;
    FlyCapture2::Camera cam;
    FlyCapture2::TriggerMode triggerMode;

    bool isCapturing;
    bool triggerEnabled;
    PixelFormat vPixFmt;

    FlyCapture2::StrobeControl strobeControl, edgeStrobeControl;

    int lastGPIOPinState;

    AVFrame *currentFrame_RAW;
    void *currentFrame_RAW_buf;
    AVFrame *currentFrame_RGB;
    struct SwsContext *sws_ctx;


signals:

public slots:

};

#endif // PTGREYINTERFACE_H
