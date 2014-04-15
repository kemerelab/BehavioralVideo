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
        config.highPerformanceRetrieveBuffer = true;

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
            vPixFmt = PIX_FMT_GRAY8;
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
                vPixFmt = PIX_FMT_UYVY422; //This is incorrect - should be bayer
            }

            break;
        default:
            qCritical() << "Unspported video mode set.";
        }
        currentFrame = new QImage(width, height, QImage::Format_RGB888);
        if (!currentFrame)
            qCritical() << "QImage not allocated";
        /*
        currentFrame_RAW = avcodec_alloc_frame();
        if (!currentFrame_RAW)
            qCritical() << "Could not allocate frame";

        sws_ctx = sws_getContext(width, height, vPixFmt, width, height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

        currentFrame_RGB = avcodec_alloc_frame();
        if (!currentFrame_RGB)
            qDebug() << "Could not allocate frame";
        avpicture_fill((AVPicture*)currentFrame_RGB, currentFrame->bits(), PIX_FMT_RGB24, width, height);
*/
        //qDebug() << "Thread for ptgrey initialize: " << QThread::currentThreadId();

        FlyCapture2::EmbeddedImageInfo embeddedInfo;
        embeddedInfo.GPIOPinState.onOff = true;
        cam.SetEmbeddedImageInfo(&embeddedInfo);

        strobeControl.duration = 5.0;
        strobeControl.polarity = 1.0;
        strobeControl.source = 1; // GPIO 1
        strobeControl.onOff = false;
        cam.SetStrobe(&strobeControl, false);

        strobeControl.duration = 5.0;
        strobeControl.polarity = 0;
        strobeControl.source = 2; // GPIO 2
        strobeControl.onOff = true;
        cam.SetStrobe(&strobeControl, false);

        triggerMode.source = 0; // GPIO 0
        triggerMode.mode = 0;
        triggerMode.onOff = true; // async
        cam.SetTriggerMode(&triggerMode, false);

        lastGPIOPinState = 0;

        // Start capturing images
        StartCapture(false); // No strobe initially.
    }

}

void PtGreyInterface::FrameReceived(FlyCapture2::Image pImage)
{
    memcpy(currentFrame->bits(), pImage.GetData(), pImage.GetDataSize());
    /*
    int ret = avpicture_fill((AVPicture*)currentFrame_RAW,
                   (uint8_t*)(pImage.GetData()), vPixFmt, width, height);
    sws_scale(sws_ctx, currentFrame_RAW->data, currentFrame_RAW->linesize, 0, height,
              currentFrame_RGB->data, currentFrame_RGB->linesize);
              */
    static FlyCapture2::ImageMetadata imageMetadata;
    imageMetadata = pImage.GetMetadata();

    if (lastGPIOPinState != imageMetadata.embeddedGPIOPinState) {
        qDebug() << "Changed GPIO value. Last: " << lastGPIOPinState << " Current: " << imageMetadata.embeddedGPIOPinState;
        lastGPIOPinState = imageMetadata.embeddedGPIOPinState;
    }

    emit newFrame(*currentFrame);
    if (strobeEnabled)
        qDebug() << "Got a frame";
    //qDebug() << "Thread for SLOT: " << QThread::currentThreadId();

}

void PtGreyInterface::StartCapture(bool enableStrobe)
{
    qDebug() << "Starting capture...";
    strobeEnabled = enableStrobe;

    if (strobeEnabled) {
        qDebug() << "Starting capture... [switching to async]";
        triggerMode.onOff = true;
        FlyCapture2::Error error = cam.SetTriggerMode(&triggerMode,false);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
        }
        qDebug() << "Enabling GPIO 1 strobe and switching GPIO 2 to high";
        strobeControl.onOff = true;
        strobeControl.polarity = 1;
        strobeControl.source = 2;
        cam.SetStrobe(&strobeControl, false);
        strobeControl.onOff = true;
        strobeControl.source = 1;
        strobeControl.polarity = 1;
        cam.SetStrobe(&strobeControl, false);
        //cam.WriteRegister(0x1120,0x8000);
    }
    else {
        qDebug() << "Setting GPIO 2 to low polarity and disabling GPIO 1";
        strobeControl.onOff = false;
        strobeControl.source = 1;
        strobeControl.polarity = 1;
        cam.SetStrobe(&strobeControl, false);
        strobeControl.onOff = true;
        strobeControl.polarity = 0;
        strobeControl.source = 2;
        cam.SetStrobe(&strobeControl, false);
        //cam.WriteRegister(0x1120,0x4000);
    }

    FlyCapture2::Error error = cam.StartCapture(OnImageGrabbed, this);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        error.PrintErrorTrace();
    }
    else {
        isCapturing = true;
        emit capturingStarted();
        // this has to go to the image acquisition function where the strobes can be seen
    }

    qDebug() << "Starting capture... [switching to sync]";
    triggerMode.onOff = false;
    error = cam.SetTriggerMode(&triggerMode,false);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        error.PrintErrorTrace();
    }
}

void PtGreyInterface::StartCaptureNoStrobe()
{
    StartCapture(false);
}

void PtGreyInterface::StartCaptureWithStrobe()
{
    StartCapture(true);
}


void PtGreyInterface::StopCapture()
{
    qDebug() << "Stopping capture... [switching to async]";
    triggerMode.onOff = true;
    FlyCapture2::Error error = cam.SetTriggerMode(&triggerMode,false);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        error.PrintErrorTrace();
    }

    if (strobeEnabled) {
        qDebug() << "Setting GPIO 2 to low polarity and disabling GPIO 1";
        strobeControl.source = 1;
        strobeControl.polarity = 1;
        strobeControl.onOff = false;
        cam.SetStrobe(&strobeControl, false);
        strobeControl.source = 2;
        strobeControl.onOff = true;
        strobeControl.polarity = 0;
        cam.SetStrobe(&strobeControl, false);
    }

    qDebug() << "Stopping capture... [stopping capture]";
    error = cam.StopCapture();
    if (error != FlyCapture2::PGRERROR_OK)
    {
        error.PrintErrorTrace();
    }
    else {
        isCapturing = false;
        emit capturingEnded();
    }
}

void OnImageGrabbed(FlyCapture2::Image* pImage, const void* pCallbackData)
{
    FlyCapture2::Image nImage;
    //FlyCapture2::Error error = nImage.DeepCopy(pImage);
    FlyCapture2::Error error;

    static FlyCapture2::ImageMetadata im;
    static unsigned int last = 0;
    im = pImage->GetMetadata();

    //if (last != im.embeddedGPIOPinState) {
        qDebug() << "[]GPIO value. Last: " << hex << last << " Current: " << hex << im.embeddedGPIOPinState;
        last = im.embeddedGPIOPinState;
    //}


    //pImage->Convert(FlyCapture2::PIXEL_FORMAT_RGB8, &nImage);
    FlyCapture2::Image::SetDefaultColorProcessing(FlyCapture2::NEAREST_NEIGHBOR);
    FlyCapture2::Image::SetDefaultOutputFormat(FlyCapture2::PIXEL_FORMAT_RGB);
    pImage->Convert(&nImage);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        //error.PrintErrorTrace();
        //qDebug() << "Error in convert" << error.GetDescription();
    }
    PtGreyInterface *pgInt = (PtGreyInterface *) pCallbackData;
    qRegisterMetaType<FlyCapture2::Image>("FlyCapture2::Image");
    QMetaObject::invokeMethod(pgInt, "FrameReceived", Qt::QueuedConnection,
                              Q_ARG(FlyCapture2::Image, nImage));

    //qDebug() << "Thread for callback: " << QThread::currentThreadId();
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
