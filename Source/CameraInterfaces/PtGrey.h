#ifndef PTGREYINTERFACE_H
#define PTGREYINTERFACE_H

#include <QObject>
#include "FFMPEG.h"
#include <FlyCapture2.h>
#include <QImage>
#include "GenericCamera.h"


#define MAX_CAMERAS 10


void OnImageGrabbed(FlyCapture2::Image* pImage, const void* pCallbackData);

void FindPointGreyCameras(QStringList *cameraNameList);

Q_DECLARE_METATYPE(FlyCapture2::Image);

class PtGreyInterface : public GenericCameraInterface
{
    Q_OBJECT
public:
    explicit PtGreyInterface(QObject *parent = 0);
     ~PtGreyInterface(void);
    bool isochStarted;

public slots:
    void Initialize();
    void StartCapture(bool enableStrobe);
    void StopCapture(void);

    void FrameReceived(FlyCapture2::Image pImage);
    void ChangeTriggerPin(int);
public:
    unsigned int serialNumber;

private:
    //FlyCapture2::PixelFormat pixFmt;
    //PixelFormat vPixFmt;

    FlyCapture2::Image *pImage; //PtGrey format
    FlyCapture2::VideoMode videoMode;
    FlyCapture2::FrameRate frameRate;
    FlyCapture2::Property shutter;
    FlyCapture2::Property gain;
    FlyCapture2::Camera cam;
    FlyCapture2::TriggerMode triggerMode;

    bool triggerEnabled;

};

#endif // PTGREYINTERFACE_H
