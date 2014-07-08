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

Serial::~Serial()
{

}

