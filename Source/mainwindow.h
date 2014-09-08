#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QSignalMapper>
#include <QMainWindow>
#include <QGridLayout>
#include "videowidget.h"
#include "videoglwidget.h"
#include "videowriter.h"
#include "ptgreyinterface.h"
#include "fakecamerainterface.h"
#include <QSerialPort>
#include "serialcameracontroller.h"
#include "dummycameracontroller.h"
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

    DummyCameraController *dummyController;
    SerialCameraController *serialController;
    QHash<unsigned int, PtGreyInterface *> cameraInterfaces;
    uint widgetx;
    uint widgety;
    uint numCameras;

public slots:
    void openSerialController(void);
    void openDummyController(void);
    void openFakeVideo();
    void openPGCamera(int serialNumber);
    void openCamera(GenericCameraInterface *camera);
    void selectPin(QString);

    void updateVideoSavingMenus(bool writing);
    void controlVideoWriter();

    void openVideoFile();
    void aggregateVideoWritingInitialized();
    void aggregateVideoWritingStarted();
    void aggregateVideoWritingEnded();
    void aggregateVideoCaptureStarted();
    void aggregateVideoCaptureEnded();

    void startCaptureAsync();
    void startCaptureSync();
    void restartCaptureAsync();
    void restartCaptureSync();

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

    enum SavingState {
        NOT_SAVING,
        READY_TO_WRITE,
        CURRENTLY_WRITING
    };
    SavingState savingState;
    uint numCamerasReadyToWrite;
    uint numCamerasInitialized;
    uint numCamerasCapturing;
    bool dummyControllerSelected;
    enum CameraState {
        SYNC,
        ASYNC
    };
    CameraState cameraState;
    QSignalMapper* cameraMapper;
    QSignalMapper* controllerMapper;
};

#endif // MAINWINDOW_H
