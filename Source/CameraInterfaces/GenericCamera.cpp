#include "GenericCamera.h"

#include <QDebug>
#include <QThread>

#include <QtCore/QString>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>

GenericCameraInterface::GenericCameraInterface(QObject *parent) :
    QObject(parent)
{
    isCapturing = false;
    isInitialized = false;
    width = -1;
    height = -1;

    cameraName = "";
}

GenericCameraInterface::~GenericCameraInterface()
{
}
