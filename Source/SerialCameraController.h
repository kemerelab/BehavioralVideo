#ifndef SERIAL_CAMERA_CONTROLLER_H
#define SERIAL_CAMERA_CONTROLLER_H

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include "GenericCameraController.h"

bool isSerialControllerConnected();

class SerialCameraController : public GenericCameraController
{

    Q_OBJECT
public:
    QString portname;
    QSerialPort port;
    ~SerialCameraController();
    int connect(QString portname);

public slots:
    void startTrigger(bool syncState);
    void stopTrigger(void);

signals:

};

#endif
