#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QSignalMapper>
#include <QMainWindow>
#include <QGridLayout>
#include "datacontroller.h"
#include "videowidget.h"
#include "videoglwidget.h"
#include "videowriter.h"
#include "ptgreyinterface.h"
#include "fakecamerainterface.h"
#include <QSerialPort>
#include "GenericCameraController.h"
#include "MazeInfoWindow.h"

#include <QHash>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    GenericCameraController *controller;

    QHash<unsigned int, PtGreyInterface *> cameraInterfaces;
    uint widgetx;
    uint widgety;
    uint numCameras;

public slots:
    void openMazeController(void);
    void openDummyController(void);
    void openController();
    void openFakeVideo();
    void openPGCamera(int serialNumber);
    void openCamera(GenericCameraInterface *camera);
    void selectPin(QString);

    void updateVideoMenus(SavingState state);

    void openVideoFile();

    void resetSavingMenus();

signals:
    //void initializeController(QString portname);
    void initializeVideoWriting(QString filename);
    void videoWritingInitialized(void);
    void videoCaptureStartedSync(void);
    void videoCaptureStartedAsync(void);
    void videoCaptureEnded(void);
    void videoWritingStarted(void);
    void videoWritingEnded(void);
    void startVideoWriting(void);
    void endVideoWriting(void);
    void startCaptureAsyncSignal(void);
    void startCaptureSyncSignal(void);
    void restartCaptureAsyncSignal(void);
    void restartCaptureSyncSignal(void);

private slots:

private:
    Ui::MainWindow *ui;
    QGridLayout *layout;
    QString name;

    DataController *dataController;
    SavingState savingState;
    uint numCamerasReadyToWrite;
    uint numCamerasInitialized;
    uint numCamerasCapturing;
    bool controllerInitialized;
    enum CameraState {
        SYNC,
        ASYNC
    };
    CameraState cameraState;
    TriggerType triggerType;

    QSignalMapper* cameraMapper;
    QSignalMapper* controllerMapper;
};

#endif // MAINWINDOW_H
