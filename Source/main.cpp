

#include "mainwindow.h"
#include <QApplication>
#include <QThread>
#include <QDebug>

QT_USE_NAMESPACE

QThread videoWriterThread0;
QThread videoWriterThread1;
QThread dataControllerThread;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //qDebug() << "Thread for main: " << QThread::currentThreadId();

    videoWriterThread0.start();
    videoWriterThread0.setObjectName("VideoWriter0Thread");
    videoWriterThread1.start();
    videoWriterThread1.setObjectName("VideoWriter1Thread");
    dataControllerThread.start();
    dataControllerThread.setObjectName("DataControllerThread");


    MainWindow w;
    QObject::connect(&w, SIGNAL(destroyed()), &videoWriterThread0, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &videoWriterThread1, SLOT(quit()));
    QObject::connect(&w, SIGNAL(destroyed()), &dataControllerThread, SLOT(quit()));

    w.show();
    return a.exec();
}
