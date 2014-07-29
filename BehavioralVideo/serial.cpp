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
}

void Serial::startTrigger(bool syncState) {
    if (syncState)
        port.write("all\r");
    else {
        port.write("cam\r");
        qDebug() << "cam enabled";
    }
}

void Serial::startTriggerNoSync() {
    startTrigger(false);
}

void Serial::startTriggerSync() {
    startTrigger(true);
}

Serial::~Serial()
{

}

