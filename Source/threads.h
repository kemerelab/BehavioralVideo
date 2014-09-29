#ifndef THREADS_H
#define THREADS_H

#include <QThread>

extern QThread cameraThread0;
extern QThread cameraThread1;
extern QThread videoWriterThread0;
extern QThread videoWriterThread1;
extern QThread dataControllerThread;
extern QThread cameraControllerThread;

#endif // THREADS_H
