

#include "mainwindow.h"
#include <QApplication>
#include <QThread>
#include <QTimer>
#include <QDebug>







QT_USE_NAMESPACE

QThread cameraThread0;
QThread cameraThread1;
QThread videoWriterThread0;
QThread videoWriterThread1;
QThread dataControllerThread;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //qDebug() << "Thread for main: " << QThread::currentThreadId();

    cameraThread0.start();
    cameraThread1.start();
    videoWriterThread0.start();
    videoWriterThread1.start();
    dataControllerThread.start();


    MainWindow w;
    QObject::connect(&w, SIGNAL(destroyed()), &cameraThread0, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &cameraThread1, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &videoWriterThread0, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &videoWriterThread1, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &dataControllerThread, SLOT(quit()));

    w.show();
    return a.exec();
}
