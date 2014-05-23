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
        config.grabTimeout = 500;
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

        strobeControl.duration = 15.0;
        strobeControl.polarity = 1.0;
        strobeControl.source = 3; // GPIO 1
        strobeControl.onOff = false;
        strobeControl.delay = 0;
        cam.SetStrobe(&strobeControl, false);

        edgeStrobeControl.duration = 5.0;
        edgeStrobeControl.polarity = 0;
        edgeStrobeControl.source = 2; // GPIO 2
        edgeStrobeControl.onOff = true;
        cam.SetStrobe(&edgeStrobeControl, false);

        triggerMode.source = 1; // GPIO 0
        triggerMode.mode = 0;
        triggerMode.onOff = false; // start in async mode
        cam.SetTriggerMode(&triggerMode, false);


        // Capture a frame just to make sure...
        FlyCapture2::Image tmpImage;
        error = cam.StartCapture(NULL, this);
        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
            qDebug() << "Missed error?";
        }file://
        else {
            unsigned int regVal;
            qDebug() << "Trigger a shot";
            do
            {
                error = cam.ReadRegister( 0x62C, &regVal );
                if (error != FlyCapture2::PGRERROR_OK)
                {
                    error.PrintErrorTrace();
                }

            } while ( (regVal >> 31) != 0 );
            cam.WriteRegister(0x62C,0x80000000); // software trigger
            error = cam.RetrieveBuffer(&tmpImage);
            error = cam.RetrieveBuffer(&tmpImage); // this should time out
            if (error != FlyCapture2::PGRERROR_OK)
            {
                error.PrintErrorTrace();
                qDebug() << "Expected timeout?";
            }
        }
        error = cam.StopCapture();
        if (error != FlyCapture2::PGRERROR_OK)
        {
            error.PrintErrorTrace();
        }


        lastGPIOPinState = 0;

        // Start capturing images
        StartCapture(false); // No strobe initially.
    }

}

//void PtGreyInterface::FrameReceived(FlyCapture2::Image pImage)
void PtGreyInterface::FrameReceived(ImageWithMetadata img)
{
    memcpy(currentFrame->bits(), img.flyCapImage.GetData(), img.flyCapImage.GetDataSize());
    /*
    int ret = avpicture_fill((AVPicture*)currentFrame_RAW,
                   (uint8_t*)(pImage.GetData()), vPixFmt, width, height);
    sws_scale(sws_ctx, currentFrame_RAW->data, currentFrame_RAW->linesize, 0, height,
              currentFrame_RGB->data, currentFrame_RGB->linesize);
              */
    //static unsigned int last = 0;
    //qDebug() << "[]GPIO value. Last: " << hex << last << " Current: " << hex << img.flag;
    //last = img.flag;

    bool strobed = ((unsigned int)img.flag & 0x20000000) == 0;

    emit newFrame(*currentFrame);

    unsigned int regVal;
    if (strobeEnabled) {
        /*do
        {
            cam.ReadRegister( 0x62C, &regVal );
        } while ( (regVal >> 31) != 0 );
        cam.WriteRegister(0x62C,0x80000000); // software trigger */

        qDebug() << "Strobed?" << hex << strobed;
    }


    //qDebug() << "Thread for SLOT: " << QThread::currentThreadId();

}

void PtGreyInterface::StartCapture(bool enableStrobe)
{
    qDebug() << "Starting capture...";
    strobeEnabled = enableStrobe;

    FlyCapture2::Error error;

    FlyCapture2::Image tmpImage;

    if (strobeEnabled) {
        qDebug() << "Starting capture... [switching to async]";
        //cam.WriteRegister(0x0830,0x82100000); // trigger ON

        triggerMode.onOff = true;
        error = cam.SetTriggerMode(&triggerMode,false);
        //if (error != FlyCapture2::PGRERROR_OK)
        //{
        //    error.PrintErrorTrace();
        //}
        qDebug() << "Enabling GPIO 1 strobe";
        //cam.WriteRegister(0x1120,0x80030000); // figured out from Flycap2
        strobeControl.onOff = true;
        cam.SetStrobe(&strobeControl, false);
        //edgeStrobeControl.polarity = 1;
        //cam.SetStrobe(&edgeStrobeControl, false);

    }
    else {
        qDebug() << "disabling GPIO 1";
        //cam.WriteRegister(0x1120,0x80040000); // figured out from Flycap2 (PWM mode??)
        strobeControl.onOff = false;
        cam.SetStrobe(&strobeControl, false);
        //edgeStrobeControl.polarity = 0;
        //cam.SetStrobe(&edgeStrobeControl, false);
    }

    //if (strobeEnabled) {
    //    cam.WriteRegister(0x62C,0x80000000); // software trigger
    //}
    //else {
        qDebug() << "Starting capture... [switching to sync]";
        cam.WriteRegister(0x0830,0x80100000); // trigger off
        //triggerMode.onOff = false;
        //error = cam.SetTriggerMode(&triggerMode,false);
        //if (error != FlyCapture2::PGRERROR_OK)
        //{
        //    error.PrintErrorTrace();
        //}
    //}

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
    FlyCapture2::Error error;
    qDebug() << "Stopping capture... [switching to async]";
    cam.WriteRegister(0x0830,0x82100000); // trigger ON
    //triggerMode.onOff = true;
    //FlyCapture2::Error error = cam.SetTriggerMode(&triggerMode,false);
    //if (error != FlyCapture2::PGRERROR_OK)
    //{
    //    error.PrintErrorTrace();
    //}

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
    //FlyCapture2::Image nImage;
    ImageWithMetadata nImage;
    //FlyCapture2::Error error = nImage.DeepCopy(pImage);
    FlyCapture2::Error error;

    static FlyCapture2::ImageMetadata im;
    static unsigned int last = 0;
    im = pImage->GetMetadata();

    nImage.flag = im.embeddedGPIOPinState;

    //pImage->Convert(FlyCapture2::PIXEL_FORMAT_RGB8, &nImage);
    FlyCapture2::Image::SetDefaultColorProcessing(FlyCapture2::NEAREST_NEIGHBOR);
    FlyCapture2::Image::SetDefaultOutputFormat(FlyCapture2::PIXEL_FORMAT_RGB);
    pImage->Convert(&nImage.flyCapImage);
    if (error != FlyCapture2::PGRERROR_OK)
    {
        //error.PrintErrorTrace();
        //qDebug() << "Error in convert" << error.GetDescription();
    }
    PtGreyInterface *pgInt = (PtGreyInterface *) pCallbackData;
    //qRegisterMetaType<FlyCapture2::Image>("FlyCapture2::Image");
    qRegisterMetaType<ImageWithMetadata>("ImageWithMetadata");
    QMetaObject::invokeMethod(pgInt, "FrameReceived", Qt::QueuedConnection,
                              Q_ARG(ImageWithMetadata, nImage));
//    Q_ARG(FlyCapture2::Image, nImage));

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
