

#include "mainwindow.h"
#include <QApplication>
#include <QThread>
#include <QDebug>

QT_USE_NAMESPACE

QThread cameraThread0;
QThread cameraThread1;
QThread videoWriterThread0;
QThread videoWriterThread1;
QThread dataControllerThread;
QThread cameraControllerThread;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //qDebug() << "Thread for main: " << QThread::currentThreadId();

    cameraThread0.setObjectName("Camera0Thread");
    cameraThread0.start();
    cameraThread1.setObjectName("Camera1Thread");
    cameraThread1.start();
    videoWriterThread0.start();
    videoWriterThread0.setObjectName("VideoWriter0Thread");
    videoWriterThread1.start();
    videoWriterThread1.setObjectName("VideoWriter1Thread");
    dataControllerThread.start();
    dataControllerThread.setObjectName("DataControllerThread");
    cameraControllerThread.start();
    cameraControllerThread.setObjectName("CameraControllerThread");

    MainWindow w;
    QObject::connect(&w, SIGNAL(destroyed()), &cameraThread0, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &cameraThread1, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &videoWriterThread0, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &videoWriterThread1, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &dataControllerThread, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &cameraControllerThread, SLOT(quit()));

    w.show();
    return a.exec();
}
