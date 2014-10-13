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
        connect(this, SIGNAL(newFrame(QVideoFrame)), videoWidget, SLOT(newFrame(QImage)));
        connect(this, SIGNAL(newFrame(QVideoFrame)), videoWriter, SLOT(newFrame(QImage)));
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

void DataController::newLeftFrame(QImage leftFrame)
{
    if (!concatenatingFrameInitialized) {
        concatenatingFrame = new QImage(leftFrame.width()*2, leftFrame.height(), QImage::Format_RGB888);
        concatenationPainter = new QPainter(concatenatingFrame);
        qDebug() << "Concatenating frame initialized" << leftFrame.width() * 2 << " " << leftFrame.height();
        concatenatingFrameInitialized = true;
    }

    if (frameConcatenationState == NOT_STARTED) {
        concatenationPainter->drawImage(0,0,leftFrame);
        frameConcatenationState = LEFT_READY;
    } else if (frameConcatenationState == LEFT_READY) {
        concatenationPainter->drawImage(0,0,leftFrame);
        frameConcatenationState = LEFT_READY;
        qDebug() << "Two left frames received before a right frame";
    } else if (frameConcatenationState == RIGHT_READY) {
        concatenationPainter->drawImage(0,0,leftFrame);
        frameConcatenationState = NOT_STARTED;
        emit newFrame(*concatenatingFrame);
    }
}

void DataController::newRightFrame(QImage rightFrame)
{
    if (!concatenatingFrameInitialized) {
        concatenatingFrame = new QImage(rightFrame.width()*2, rightFrame.height(), QImage::Format_RGB888);
        concatenationPainter = new QPainter(concatenatingFrame);
        qDebug() << "Concatenating frame initialized" << rightFrame.width() * 2 << " " << rightFrame.height();
        concatenatingFrameInitialized = true;
    }

    if (frameConcatenationState == NOT_STARTED) {
        concatenationPainter->drawImage(rightFrame.width(),0,rightFrame);
        frameConcatenationState = RIGHT_READY;
    } else if (frameConcatenationState == RIGHT_READY) {
        concatenationPainter->drawImage(rightFrame.width(),0,rightFrame);
        frameConcatenationState = RIGHT_READY;
        qDebug() << "Two right frames received before a left frame";
    } else if (frameConcatenationState == LEFT_READY) {
        concatenationPainter->drawImage(rightFrame.width(),0,rightFrame);
        frameConcatenationState = NOT_STARTED;
        emit newFrame(*concatenatingFrame);
    }
}
