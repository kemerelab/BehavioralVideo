#include "DataController.h"
#include "Threads.h"
#include <QDebug>

DataController::DataController(VideoWriter* writer, QObject *parent) :
    QObject(parent)
{
    numCameras = 0;
    triggerType = NO_SELECTION;
    cameraController = NULL;

    videoIsStreaming = false;

    frameConcatenationState = NOT_STARTED;
    concatenatingFrameInitialized = false;

    videoWriter = writer;
}


void DataController::stopVideo()
{
    qDebug() << "DataController, stop Video";

    if (videoIsStreaming) {
        for (int i = 0; i < numCameras; i++) {
            QMetaObject::invokeMethod(cameraList.at(i), "StopCapture", Qt::BlockingQueuedConnection);
        }
        if (cameraController != NULL)
            QMetaObject::invokeMethod(cameraController, "stopTrigger", Qt::BlockingQueuedConnection);
        videoIsStreaming = false;
    }
}

void DataController::stopVideoWriting()
{
    stopVideo();
    QMetaObject::invokeMethod(videoWriter, "endWriting", Qt::BlockingQueuedConnection);
    startVideoStreaming(false);
    emit updateSavingMenus(NOT_SAVING);

}


void DataController::initializeVideoWriting(QString filename)
{
    QMetaObject::invokeMethod(videoWriter, "initialize", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, (QString)(filename)));
    emit updateSavingMenus(READY_TO_WRITE);
}

void DataController::startVideoStreaming(bool writeToDisk)
{
    stopVideo();

    if (writeToDisk) {
        QMetaObject::invokeMethod(videoWriter, "beginWriting", Qt::BlockingQueuedConnection);
    }
    if ((cameraController != NULL) && (triggerType == EXTERNAL_CAMERA_TRIGGER)) {
        QMetaObject::invokeMethod(cameraController, "startTriggerSync", Qt::BlockingQueuedConnection);
        for (int i = 0; i < numCameras; i++) {
            QMetaObject::invokeMethod(cameraList.at(i), "StartCameraCaptureSync", Qt::BlockingQueuedConnection);
        }
    }
    else if ((triggerType == NO_CAMERA_TRIGGER) || (cameraController == NULL)) {
        for (int i = 0; i < numCameras; i++) {
            QMetaObject::invokeMethod(cameraList.at(i), "StartCameraCaptureAsync", Qt::BlockingQueuedConnection);
        }
    }

    if (writeToDisk) {
        emit updateSavingMenus(CURRENTLY_WRITING);
    }

    videoIsStreaming = true;

}


void DataController::registerCamera(GenericCameraInterface *camera)
{

    cameraList.append(camera);
    numCameras++;

    if (numCameras == 1) {
        connect(camera, SIGNAL(newFrame(QVideoFrame)), videoWidget, SLOT(newFrame(QVideoFrame)));
        connect(camera, SIGNAL(newFrame(QVideoFrame)), videoWriter, SLOT(newFrame(QVideoFrame)));
    } else if (numCameras == 2) {
        qDebug() << "Deregistering";
        disconnect(cameraList.at(0), SIGNAL(newFrame(QVideoFrame)), videoWidget, SLOT(newFrame(QVideoFrame)));
        disconnect(cameraList.at(0), SIGNAL(newFrame(QVideoFrame)), videoWriter, SLOT(newFrame(QVideoFrame)));
        qDebug() << "Re-registering";
        connect(this, SIGNAL(newFrame(QVideoFrame)), videoWidget, SLOT(newFrame(QVideoFrame)));
        connect(this, SIGNAL(newFrame(QVideoFrame)), videoWriter, SLOT(newFrame(QVideoFrame)));
        connect(cameraList.at(0), SIGNAL(newFrame(QVideoFrame)), this, SLOT(newLeftFrame(QVideoFrame)));
        connect(cameraList.at(1), SIGNAL(newFrame(QVideoFrame)), this, SLOT(newRightFrame(QVideoFrame)));
    }

    qDebug() << "Camera registered " << camera->cameraName << " #" << numCameras;
}

void DataController::registerCameraController(GenericCameraController *controller)
{
    cameraController = controller;
    qDebug() << "Camera controller registered ";
}

void DataController::registerVideoWidget(VideoGLWidget *_videoWidget)
{
    videoWidget = _videoWidget;
}

void DataController::newLeftFrame(QVideoFrame frame)
{
    concatenateFrames(left, frame);
}

void DataController::newRightFrame(QVideoFrame frame)
{
    concatenateFrames(right, frame);
}

void DataController::concatenateFrames(DataController::WhichFrame which, QVideoFrame frame)
{
    if (!concatenatingFrameInitialized) {
        int width = frame.width();
        int height = frame.height();
        //concatenatingImage = new QImage(width*2, height, QImage::Format_RGB888);
        //concatenationPainter = new QPainter(concatenatingImage);
        concatenatingFrame = new QVideoFrame(width * 2 * height * 3,
                                             QSize(width*2,height), width*2, QVideoFrame::Format_RGB24);
        qDebug() << "Creating a concatenating frame of size " << 2*width << " x " << height;
        concatenatingFrameInitialized = true;
    }

    if (!frame.map(QAbstractVideoBuffer::ReadOnly))
        qDebug() << "Failed to map current frame";
    else {
        if (!concatenatingFrame->map(QAbstractVideoBuffer::WriteOnly))
            qDebug() << "Failed to map concatenating frame";
        else {
            //concatenationPainter->drawImage(frame.width() * (which==right),0,frame);
            for (int i=0; i < frame.height(); i++)
                memcpy(concatenatingFrame->bits() + concatenatingFrame->width()*3*i
                       + frame.width()*3*(which==right),
                       frame.bits() + frame.width()*3*i, frame.width()*3);
            concatenatingFrame->unmap();

            if (frameConcatenationState == NOT_STARTED) {
                frameConcatenationState = (which==left) ? LEFT_READY : RIGHT_READY;
            } else if (frameConcatenationState == LEFT_READY) {
                if (which == left)
                    qDebug() << "Two left frames received before a right frame";
                else {
                    frameConcatenationState = NOT_STARTED;
                    emit newFrame(*concatenatingFrame);
                }
            } else if (frameConcatenationState == RIGHT_READY) {
                if (which == right)
                    qDebug() << "Two right frames received before a right frame";
                else {
                    frameConcatenationState = NOT_STARTED;
                    emit newFrame(*concatenatingFrame);
                }
            }
        }
        frame.unmap();
    }


}
