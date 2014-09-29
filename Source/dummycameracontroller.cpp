#include "dummycameracontroller.h"
#include <QDebug>

DummyCameraController::DummyCameraController(QObject *parent) :
    GenericCameraController(parent)
{

}

void DummyCameraController::stopTrigger()
{

}

void DummyCameraController::startTrigger(bool syncState) {
    if (syncState) {
        qDebug() << "triggers started true";
    }
    else {
        qDebug() << "triggers started false";
    }
}

DummyCameraController::~DummyCameraController()
{

}
