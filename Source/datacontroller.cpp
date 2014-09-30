#include "datacontroller.h"
#include "threads.h"
#include <QDebug>

DataController::DataController(QObject *parent) :
    QObject(parent)
{
    numCameras = 0;
    triggerType = NO_SELECTION;
    cameraController = NULL;

    videoIsStreaming = false;

    frameConcatenationState = NOT_STARTED;
    concatenatingFrameInitialized = false;
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
    for (int i = 0; i < videoWriterList.length(); i++) {
        QMetaObject::invokeMethod(videoWriterList.at(i), "endWriting", Qt::BlockingQueuedConnection);
    }
    startVideoStreaming(false);
    emit updateSavingMenus(NOT_SAVING);

}


void DataController::initializeVideoWriting(QString filename)
{
    QMetaObject::invokeMethod(videoWriterList.at(0), "initialize", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, (QString)(filename)));
    emit updateSavingMenus(READY_TO_WRITE);
}

void DataController::startVideoStreaming(bool writeToDisk)
{
    stopVideo();

    if (writeToDisk) {
        for (int i = 0; i < videoWriterList.length(); i++) {
            QMetaObject::invokeMethod(videoWriterList.at(i), "beginWriting", Qt::BlockingQueuedConnection);
        }

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
        VideoWriter *writer = new VideoWriter();
        writer->moveToThread(&videoWriterThread0);
        videoWriterList.append(writer);

        connect(camera, SIGNAL(newFrame(QImage)), videoWidget, SLOT(newFrame(QImage)));
        connect(camera, SIGNAL(newFrame(QImage)), writer, SLOT(newFrame(QImage)));
    } else if (numCameras == 2) {
        qDebug() << "Deregistering";
        disconnect(cameraList.at(0), SIGNAL(newFrame(QImage)), videoWidget, SLOT(newFrame(QImage)));
        disconnect(cameraList.at(0), SIGNAL(newFrame(QImage)), videoWriterList.at(0), SLOT(newFrame(QImage)));
        qDebug() << "Re-registering";
        connect(this, SIGNAL(newFrame(QImage)), videoWidget, SLOT(newFrame(QImage)));
        connect(this, SIGNAL(newFrame(QImage)), videoWriterList.at(0), SLOT(newFrame(QImage)));
        connect(cameraList.at(0), SIGNAL(newFrame(QImage)), this, SLOT(newLeftFrame(QImage)));
        connect(cameraList.at(1), SIGNAL(newFrame(QImage)), this, SLOT(newRightFrame(QImage)));
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
