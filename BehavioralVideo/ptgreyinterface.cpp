#include "ptgreyinterface.h"
#include <QDebug>
#include <QThread>


PtGreyInterface::PtGreyInterface(QObject *parent) :
    QObject(parent)
{
    isCapturing = false;
}

void PtGreyInterface::Initialize()
{
    FlyCapture2::Error error;
    FlyCapture2::BusManager busMgr;
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
        error = busMgr.GetCameraFromIndex(0, &guid);
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

        // Set the camera configuration
        error = cam.SetConfiguration( &config );
        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
        }

        qDebug() <<
            "\n*** CAMERA INFORMATION ***\n" <<
            "Serial number -" << camInfo.serialNumber <<
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
        case FlyCapture2::VIDEOMODE_640x480Y8:
            width = 640;
            height = 480;
            pixFmt = FlyCapture2::PIXEL_FORMAT_MONO8;
            break;
        case FlyCapture2::VIDEOMODE_FORMAT7:
            error = cam.GetFormat7Configuration(&fmt7settings, &pPktSize, &pPercPktSize);
            if (error != FlyCapture2::PGRERROR_OK)
            {
                error.PrintErrorTrace();
            }
            else {
                width = fmt7settings.width;
                height = fmt7settings.height;
                pixFmt = fmt7settings.pixelFormat;
            }

            break;
        default:
            qCritical() << "Unspported video mode set.";
        }


        currentFrame = new QImage(width, height, QImage::Format_RGB888);
        /*
        if (!currentFrame)
            qCritical() << "QImage not allocated";
        currentFrame_RGB = avcodec_alloc_frame();
        if (!currentFrame_RAW)
            qDebug() << "Could not allocate frame";
        avpicture_fill((AVPicture*)currentFrame_RGB, currentFrame->bits(), PIX_FMT_RGB24, width, height);

        switch (videoMode) {
        case FlyCapture2::VIDEOMODE_640x480Y8:
            vPixFmt = PIX_FMT_GRAY8;
            //vPixFmt = PIX_FMT_YUV420P;
            break;
        case FlyCapture2::VIDEOMODE_640x480YUV422:
            vPixFmt = PIX_FMT_UYVY422;
            break;
        default:
            qCritical() << "Bad formats";
        }

        currentFrame_RAW = avcodec_alloc_frame();
        if (!currentFrame_RAW)
            qCritical() << "Could not allocate frame";

        sws_ctx = sws_getContext(width, height, vPixFmt, width, height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
        //qDebug() << "Thread for ptgrey initialize: " << QThread::currentThreadId();

        */
        // Start capturing images
        StartCapture();
    }

}

void PtGreyInterface::FrameReceived(FlyCapture2::Image pImage)
{
    qDebug() << "Thread for SLOT: " << QThread::currentThreadId();
    memcpy(currentFrame->bits(), pImage.GetData(), pImage.GetDataSize());
    /*int ret = avpicture_fill((AVPicture*)currentFrame_RAW,
                   (uint8_t*)(pImage.GetData()), vPixFmt, width, height);
    sws_scale(sws_ctx, currentFrame_RAW->data, currentFrame_RAW->linesize, 0, height,
              currentFrame_RGB->data, currentFrame_RGB->linesize);*/


    emit newFrame(*currentFrame);
}

void PtGreyInterface::StartCapture()
{
    qDebug() << "Starting capture...";
    FlyCapture2::Error error = cam.StartCapture(OnImageGrabbed, this);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        error.PrintErrorTrace();
    }
    else {
        isCapturing = true;
    }
}

void PtGreyInterface::StopCapture()
{
    qDebug() << "Stopping capture...";
    FlyCapture2::Error error = cam.StopCapture();
    if (error != FlyCapture2::PGRERROR_OK)
    {
        error.PrintErrorTrace();
    }
    else {
        isCapturing = false;
    }
}

void OnImageGrabbed(FlyCapture2::Image* pImage, const void* pCallbackData)
{
    FlyCapture2::Image nImage;
    //FlyCapture2::Error error = nImage.DeepCopy(pImage);
    FlyCapture2::Error error;
    pImage->Convert(FlyCapture2::PIXEL_FORMAT_RGB8, &nImage);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        error.PrintErrorTrace();
    }
    PtGreyInterface *pgInt = (PtGreyInterface *) pCallbackData;
    qRegisterMetaType<FlyCapture2::Image>("FlyCapture2::Image*");
    QMetaObject::invokeMethod(pgInt, "FrameReceived", Qt::QueuedConnection,
                              Q_ARG(FlyCapture2::Image, nImage));

    qDebug() << "Thread for callback: " << QThread::currentThreadId();
    return;
}

PtGreyInterface::~PtGreyInterface()
{
    if (isCapturing) {
        StopCapture();
    }
    av_free(currentFrame_RAW);
    av_free(currentFrame_RGB);
}
