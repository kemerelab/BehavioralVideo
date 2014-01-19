#include "mainwindow.h"
#include "camerainterface.h"
#include "videoglwidget.h"
#include <QApplication>
#include <QThread>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QThread cameraThread;
    QObject::connect(&w, SIGNAL(destroyed()), &cameraThread, SLOT(quit()));

    QThread videoWriterThread;
    QObject::connect(&w, SIGNAL(destroyed()), &videoWriterThread, SLOT(quit()));
    // Should connect this signal to the writing process so that files get closed
    w.videoWriter->moveToThread(&videoWriterThread);
    videoWriterThread.start();

    CameraInterface cameraInterface;

    QTimer timer;
    QObject::connect(&timer, SIGNAL(timeout()), &cameraInterface, SLOT(GenerateNextFrame()));
    timer.start(30);

    timer.moveToThread(&cameraThread);
    cameraInterface.moveToThread(&cameraThread);

    QObject::connect(&cameraInterface, SIGNAL(newFrame(QImage)),w.videoWidget,
                     SLOT(newFrame(QImage)));
    QObject::connect(&cameraInterface, SIGNAL(newFrame(QImage)),w.videoWriter,
                     SLOT(newFrame(QImage)));

    cameraThread.start();

    return a.exec();
}
