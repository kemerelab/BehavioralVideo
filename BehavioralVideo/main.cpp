

#include "mainwindow.h"
#include "fakecamerainterface.h"
#include "videoglwidget.h"
#include <QApplication>
#include <QThread>
#include <QTimer>
#include <QDebug>







QT_USE_NAMESPACE

QThread pgThread;
QThread cameraThread;
QThread videoWriterThread;




int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //qDebug() << "Thread for main: " << QThread::currentThreadId();





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
