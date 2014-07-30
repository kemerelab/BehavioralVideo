#include <serial.h>
#include <QThread>




void Serial::connect(QString portname)
{

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {

           if (info.portName() == portname){
                port.setPort(info);
                port.open(QIODevice::ReadWrite);
                if (!port.isOpen()){
                    qDebug() << "Port not open";
                }
                port.setBaudRate(QSerialPort::Baud9600);
                break;
           }
    }
}

void Serial::stopTrigger()
{
    port.write("\r");
    emit triggersStopped();
}

void Serial::startTrigger(bool syncState) {
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

void Serial::startTriggerNoSync() {
    startTrigger(false);
    qDebug() << "starting trigger false";
}

void Serial::startTriggerSync() {
    startTrigger(true);
    qDebug() << "starting trigger true";
}

Serial::~Serial()
{

}

