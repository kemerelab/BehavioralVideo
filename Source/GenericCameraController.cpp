#include "GenericCameraController.h"
#include <QDebug>

GenericCameraController::GenericCameraController(QObject *parent) :
    QObject(parent)
{
    numCameras = 0;
}

GenericCameraController::~GenericCameraController()
{

}
