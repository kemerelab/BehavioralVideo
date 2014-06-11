

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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>




QT_USE_NAMESPACE

QThread pgThread;
QThread cameraThread;
QThread videoWriterThread;

char buffer[10];


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //qDebug() << "Thread for main: " << QThread::currentThreadId();



    QSerialPortInfo myinfo;

    qDebug() << "Selected Serial Port:";
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
           if (info.manufacturer() == "Texas Instruments"){

               qDebug() << "Name        : " << info.portName();
               qDebug() << "Description : " << info.description();
               qDebug() << "Manufacturer: " << info.manufacturer();
               myinfo = info;
               break;
           }
    }
    if (myinfo.portName() == ""){
        qDebug() << "No TI Serial Device Found";
    }
    QSerialPort serial(myinfo);
    serial.open(QIODevice::ReadWrite);
    serial.write("test\r");


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
