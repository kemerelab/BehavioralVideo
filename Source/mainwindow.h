#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QSignalMapper>
#include <QMainWindow>
#include <QGridLayout>
#include "DataController.h"
#include "VideoGLWidget.h"
#include "VideoWriter.h"
#include "CameraInterfaces/PtGrey.h"
#include "CameraInterfaces/FakeCamera.h"
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

    void setupPrefereuncesUI(void);

public slots:
    void showPreferencesDialog(void);
    void changeVideoFormat(int fmt);
    void changeVideoExtension(QString ext);

    void openMazeController(QString portname);
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

    void newVideoFormat(VideoCompressionFormat);
    void newVideoFileExtension(QString);

private slots:

private:
    Ui::MainWindow *ui;
    QGridLayout *layout;

    QWidget *videoContainer;

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

    QDialog* settingsDialog;
    QTabWidget* settingsContainer;
};

#endif // MAINWINDOW_H
