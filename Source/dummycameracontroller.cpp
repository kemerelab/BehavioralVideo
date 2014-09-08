#include "dummycameracontroller.h"
#include <QDebug>

DummyCameraController::DummyCameraController(QObject *parent) :
    QObject(parent)
{

}

void DummyCameraController::stopTrigger()
{
    emit triggersStopped();
}

void DummyCameraController::startTrigger(bool syncState) {
    if (syncState) {
        emit triggersStarted(true);
        qDebug() << "triggers started true";
    }
    else {
        emit triggersStarted(false);
        qDebug() << "triggers started false";
    }
}

void DummyCameraController::startTriggerNoSync() {
    startTrigger(false);
    qDebug() << "starting trigger false";
}

void DummyCameraController::startTriggerSync() {
    startTrigger(true);
    qDebug() << "starting trigger true";
}

DummyCameraController::~DummyCameraController()
{

}

