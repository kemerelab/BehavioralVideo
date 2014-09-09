#include "dummycameracontroller.h"
#include <QDebug>

DummyCameraController::DummyCameraController(QObject *parent) :
    GenericCameraController(parent)
{

}

void DummyCameraController::stopTrigger()
{
    emit triggerStopped();
}

void DummyCameraController::startTrigger(bool syncState) {
    if (syncState) {
        emit triggerStarted(true);
        qDebug() << "triggers started true";
    }
    else {
        emit triggerStarted(false);
        qDebug() << "triggers started false";
    }
    emit triggerStarted();
}

DummyCameraController::~DummyCameraController()
{

}
