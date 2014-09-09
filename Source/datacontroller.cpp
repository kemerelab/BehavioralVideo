#include "datacontroller.h"
#include <QDebug>

DataController::DataController(QObject *parent) :
    QObject(parent)
{
    numCameras = 0;
    triggerType = NO_SELECTION;
    cameraController = NULL;

    videoIsStreaming = false;
}


void DataController::stopVideo()
{
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
    for (int i = 0; i < numCameras; i++) {
        QMetaObject::invokeMethod(videoWriterList.at(i), "endWriting", Qt::BlockingQueuedConnection);
    }
    startVideoStreaming(false);
    emit updateSavingMenus(NOT_SAVING);

}


void DataController::initializeVideoWriting(QString filename)
{
    for (int i = 0; i < numCameras; i++) {
        QMetaObject::invokeMethod(videoWriterList.at(i), "initialize", Qt::BlockingQueuedConnection,
                                  Q_ARG(QString, (QString)(filename + cameraList.at(i)->cameraName)));
    }
    emit updateSavingMenus(READY_TO_WRITE);
}

void DataController::startVideoStreaming(bool writeToDisk)
{
    stopVideo();

    if (writeToDisk) {
        for (int i = 0; i < numCameras; i++) {
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

}

void DataController::registerCameraAndWriter(GenericCameraInterface *camera, VideoWriter *writer)
{
    cameraList.append(camera);
    videoWriterList.append(writer);
    numCameras++;

    qDebug() << "Camera registered " << camera->cameraName << " #" << numCameras;
}

void DataController::registerCameraController(GenericCameraController *controller)
{
    cameraController = controller;
    qDebug() << "Camera controller registered ";
}
