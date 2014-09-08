#include "serialcameracontroller.h"
#include <QThread>

bool isSerialControllerConnected()
{
    bool hardware = false;
    bool firmware = false;

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
          if (info.description().left(17) == "Camera Controller"){
                 //check for correct PCB and Firmware
                 if (info.description().mid(18,7) == "PCB 1.0"){
                     hardware = true;
                 }
                 if (info.description().mid(26,7) == "F W 1.0"){
                     firmware = true;
                 }
                 if (hardware && firmware){
                     qDebug() << "Found serial camera controller " << info.description();
                     return true;
                 }
           }
    }
    return false;
}


int SerialCameraController::connect(QString portname)
{
    bool hardware = false;
    bool firmware = false;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        if (info.description().left(17) == "Camera Controller"){
            //check for correct PCB and Firmware
            if (info.description().mid(18,7) == "PCB 1.0"){
                hardware = true;
            }
            if (info.description().mid(26,7) == "F W 1.0"){
                firmware = true;
            }
            if (hardware && firmware){
                port.setPort(info);
                port.open(QIODevice::ReadWrite);
                if (!port.isOpen()){
                    qDebug() << "Port not open";
                    return 1;
                }
                port.setBaudRate(QSerialPort::Baud9600);
                return 0;
            }
        }
    }

    qDebug() << "No serial controller found";
    return 2;
}

void SerialCameraController::stopTrigger()
{
    port.write("\r");
    emit triggersStopped();
}

void SerialCameraController::startTrigger(bool syncState) {
    if (syncState) {
        port.write("all\r");
        emit triggersStarted(true);
        qDebug() << "triggers started true";
    }
    else {
        port.write("cam\r");
        qDebug() << "cam enabled";
        emit triggersStarted(false);
        qDebug() << "triggers started false";
    }
}

void SerialCameraController::startTriggerNoSync() {
    startTrigger(false);
    qDebug() << "starting trigger false";
}

void SerialCameraController::startTriggerSync() {
    startTrigger(true);
    qDebug() << "starting trigger true";
}

SerialCameraController::~SerialCameraController()
{

}

