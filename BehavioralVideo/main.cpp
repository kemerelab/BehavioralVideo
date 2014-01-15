#include "mainwindow.h"
#include "camerainterface.h"
#include <QApplication>
#include <QThread>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QThread cameraThread;
    //QObject::connect(&cameraThread, SIGNAL(finished()), this, SLOT(quit()));

    CameraInterface cameraInterface;

    QTimer timer;
    QObject::connect(&timer, SIGNAL(timeout()), &cameraInterface, SLOT(GenerateNextFrame()));
    timer.start(1000);

    timer.moveToThread(&cameraThread);
    cameraInterface.moveToThread(&cameraThread);

    QObject::connect(&cameraInterface, SIGNAL(newFrame(QImage)),w.videoWidget,
                     SLOT(newFrame(QImage)));

    cameraThread.start();

    return a.exec();
}
