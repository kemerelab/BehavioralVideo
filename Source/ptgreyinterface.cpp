#include "ptgreyinterface.h"

#include <QDebug>
#include <QThread>

#include <QtCore/QString>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>

//unsigned int registers[590];    //for dumping camera registers to

PtGreyInterface::PtGreyInterface(QObject *parent) :
    GenericCameraInterface(parent)
{
    serialNumber = 0;
}

void PtGreyInterface::Initialize()
{
    FlyCapture2::Error error;
    FlyCapture2::BusManager busMgr;

    if (serialNumber == 0) {
        qCritical() << "No cameras opened";
    }

    unsigned int numCameras;
    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        error.PrintErrorTrace();
    }

    qDebug() << "Number of cameras detected: " << numCameras;

    // Pick camera (change from first)
    if (numCameras >= 1) {
        FlyCapture2::PGRGuid guid;
        error = busMgr.GetCameraFromSerialNumber(serialNumber, &guid);
        //error = busMgr.GetCameraFromIndex(0,&guid);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
        }

        // Connect to camera

        error = cam.Connect(&guid);

        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
        }

        // Get the camera information
        FlyCapture2::CameraInfo camInfo;
        error = cam.GetCameraInfo(&camInfo);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
        }

        // Get the camera configuration
        FlyCapture2::FC2Config config;
        error = cam.GetConfiguration( &config );
        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
        }

        // Set the grab mode to buffering
        config.grabMode = FlyCapture2::BUFFER_FRAMES;
        config.grabTimeout = 500;
        config.highPerformanceRetrieveBuffer = true;

        // Set the camera configuration
        error = cam.SetConfiguration( &config );
        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
        }

        qDebug() <<
            "\n*** CAMERA INFORMATION ***" <<
            "\nSerial number -" << camInfo.serialNumber <<
            "\nCamera model - " << camInfo.modelName <<
            "\nCamera vendor - " << camInfo.vendorName <<
            "\nSensor - " << camInfo.sensorInfo <<
            "\nResolution - " << camInfo.sensorResolution;


        error = cam.GetVideoModeAndFrameRate(&videoMode, &frameRate);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
        }

        FlyCapture2::Format7ImageSettings fmt7settings;
        unsigned int pPktSize;
        float pPercPktSize;


        switch (videoMode) {
        case FlyCapture2::VIDEOMODE_640x480Y8:{
            width = 640;
            height = 480;
            //pixFmt = FlyCapture2::PIXEL_FORMAT_MONO8;
            //vPixFmt = PIX_FMT_GRAY8;
            qDebug() << "Video mode 640x480";
            frameRate = FlyCapture2::FRAMERATE_60;
            error = cam.SetVideoModeAndFrameRate(videoMode, frameRate);
            if (error != FlyCapture2::PGRERROR_OK)
            {
                error.PrintErrorTrace();
            }
            else
                qDebug() << "Set video framerate to 60 fps";

            gain.type = FlyCapture2::GAIN;
            gain.absControl = true;
            gain.onePush = false;
            gain.onOff = true;
            gain.autoManualMode = false;
            gain.absValue = 12.0;
            cam.SetProperty(&gain, false);

            shutter.type = FlyCapture2::SHUTTER;
            shutter.autoManualMode = false;
            shutter.absValue = 16.0;
            shutter.absControl = true;
            shutter.present = true;

            error = cam.SetProperty( &shutter, false );
            break;
        }
        case FlyCapture2::VIDEOMODE_FORMAT7:
            error = cam.GetFormat7Configuration(&fmt7settings, &pPktSize, &pPercPktSize);
            if (error != FlyCapture2::PGRERROR_OK)
            {
                error.PrintErrorTrace();
            }
            else {
                width = fmt7settings.width;
                height = fmt7settings.height;
                //pixFmt = fmt7settings.pixelFormat;
                //vPixFmt = PIX_FMT_UYVY422; //This is incorrect - should be bayer
                qDebug() << "Video mode format 7";
            }

            break;
        default:
            qCritical() << "Unspported video mode set.";
        }

        currentFrame = new QImage(width, height, QImage::Format_RGB888);
        if (!currentFrame)
            qCritical() << "QImage not allocated";

        triggerMode.source = 1; // GPIO 0
        triggerMode.mode = 0;
        triggerMode.onOff = false; // start in async mode
        cam.SetTriggerMode(&triggerMode, false);

    }

    cameraName = QString::number(serialNumber);
    isInitialized = true;

}

void PtGreyInterface::ChangeTriggerPin(int pin){
    triggerMode.source = pin;
    triggerMode.mode = 0;
    cam.SetTriggerMode(&triggerMode,false);
}

void PtGreyInterface::FrameReceived(FlyCapture2::Image img)
{
    memcpy(currentFrame->bits(), img.GetData(), img.GetDataSize());
    emit newFrame(*currentFrame);
}

void PtGreyInterface::StartCapture(bool enableTrigger)
{
    triggerEnabled = enableTrigger;

    FlyCapture2::Error error;

    qDebug() << "strobe:" << triggerEnabled;

    if (triggerEnabled) {
        qDebug() << "capture starting [trigger on]";

        triggerMode.onOff = true;
        error = cam.SetTriggerMode(&triggerMode,false);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
        }
    }
    else {
        qDebug() << "capture starting [trigger off]";
        triggerMode.onOff = false;
        error = cam.SetTriggerMode(&triggerMode,false);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
        }
    }

    error = cam.StartCapture(OnImageGrabbed, this);

    if (error != FlyCapture2::PGRERROR_OK)
    {
        error.PrintErrorTrace();
    }
    else {
        isCapturing = true;
        emit capturingStarted();
    }
}

void PtGreyInterface::StopCapture()
{
    FlyCapture2::Error error;
    error = cam.StopCapture();

    if (error != FlyCapture2::PGRERROR_OK)
    {
        error.PrintErrorTrace();
    }

    else {
        isCapturing = false;
        qDebug() << "Stoping capture";
        emit capturingEnded();
    }
}

void OnImageGrabbed(FlyCapture2::Image* pImage, const void* pCallbackData)
{
    FlyCapture2::Image nImage;
    //FlyCapture2::Error error = nImage.DeepCopy(pImage);
    FlyCapture2::Error error;

    FlyCapture2::Image::SetDefaultColorProcessing(FlyCapture2::NEAREST_NEIGHBOR);
    pImage->Convert(FlyCapture2::PIXEL_FORMAT_RGB, &nImage);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        //error.PrintErrorTrace();
        //qDebug() << "Error in convert" << error.GetDescription();
    }
    PtGreyInterface *pgInt = (PtGreyInterface *) pCallbackData;
    qRegisterMetaType<FlyCapture2::Image>("FlyCapture2::Image");
    QMetaObject::invokeMethod(pgInt, "FrameReceived", Qt::QueuedConnection,
                              Q_ARG(FlyCapture2::Image, nImage));

    return;
}

PtGreyInterface::~PtGreyInterface()
{
    if (isCapturing) {
        StopCapture();
    }
}


void FindPointGreyCameras(QStringList *cameraNameList)
{
    FlyCapture2::BusManager busMgr;
    FlyCapture2::PGRGuid guid;
    FlyCapture2::CameraInfo camInfo;
    FlyCapture2::Camera cameralist[MAX_CAMERAS];
    unsigned int x;
    for (x=0; x<MAX_CAMERAS;x++)
    {
        if (busMgr.GetCameraFromIndex(x, &guid) != FlyCapture2::PGRERROR_OK)
        {
            break;
        }
        cameralist[x].Connect(&guid);
        cameralist[x].GetCameraInfo(&camInfo);
        cameraNameList->append(QString::number(camInfo.serialNumber));
        qDebug() << "Camera found " << QString::number(camInfo.serialNumber);
    }

}
