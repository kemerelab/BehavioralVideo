#include <QDebug>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QString>

//foreach serial channel{
//    add a tab in gui
//    add a function for pressing gui
//}

//void initSerial(QString name){
//    QSerialPort port;
//  http://doc-snapshot.qt-project.org/qt5-stable/qtserialport-blockingmaster-example.html
//    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()){
//        if (info.portName() == name){
//            port.setPort(info);
//        }
//    }

//    qDebug() << "opening serial port";
//    port.Baud9600;
//    port.open(QIODevice::ReadWrite);
//    port.write("test");

//}

#ifndef SERIAL_H
#define SERIAL_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>



class SerialThread : public QThread
{
    Q_OBJECT

public:
    SerialThread(QObject *parent = 0);
    ~SerialThread();

    void transaction(const QString &portName, int waitTimeout, const QString &request);
    void run();

signals:
    void response(const QString &s);
    void error(const QString &s);
    void timeout(const QString &s);

private:
    QString portName;
    QString request;
    int waitTimeout;
    QMutex mutex;
    QWaitCondition cond;
    bool quit;



};

#endif // SERIAL_H
