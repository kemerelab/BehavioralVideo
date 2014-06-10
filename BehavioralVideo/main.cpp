

#include "mainwindow.h"
#include "fakecamerainterface.h"
#include "videoglwidget.h"
#include <QApplication>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include "serial.h"

QT_USE_NAMESPACE

QThread pgThread;
QThread cameraThread;
QThread videoWriterThread;

char buffer[10];
QString portname;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //qDebug() << "Thread for main: " << QThread::currentThreadId();



    QSerialPortInfo myinfo;
    qDebug() << "Detected Serial Ports:";
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
           if (info.portName() == "ttyACM1"){
                portname = info.portName();
                qDebug() << "Name        : " << info.portName();
               qDebug() << "Description : " << info.description();
               qDebug() << "Manufacturer: " << info.manufacturer();
               myinfo = info;
               break;

            }

    }

    QSerialPort serial(myinfo);
    qDebug() << serial.portName();
    serial.open(QIODevice::ReadWrite);
    qDebug() << "error:"<< serial.error();
    serial.setBaudRate(QSerialPort::Baud9600);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);

    serial.write("test\r");
    serial.close();






    cameraThread.start();
    videoWriterThread.start();
    pgThread.start();

    MainWindow w;
    QObject::connect(&w, SIGNAL(destroyed()), &cameraThread, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &videoWriterThread, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &pgThread, SLOT(quit()));

    w.show();
    return a.exec();
}
