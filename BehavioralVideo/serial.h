
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
class Serial{
protected:

public:
    void openSerial(){
        QSerialPortInfo myinfo;
        qDebug() << "Selected Serial Port:";
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
               if (info.manufacturer() != ""){

                   qDebug() << "Name        : " << info.portName();
                   qDebug() << "Description : " << info.description();
                   qDebug() << "Manufacturer: " << info.manufacturer();
                   myinfo = info;
                   break;
               }
        }
        if (myinfo.portName() == ""){
            qDebug() << "No Serial Device Found";
        }
        QSerialPort serial(myinfo);
        serial.open(QIODevice::ReadWrite);
        serial.write("test\r");
    }

};
