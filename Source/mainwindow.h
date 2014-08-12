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
#include "serial.h"
#include <QHash>

#define MAX_CAMERAS 10

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    VideoGLWidget *videoWidget[10];
    //int cameraIndex;
    //PtGreyInterface *pgCamera[10];
    FakeVideoGenerator *fakeCamera;
    Serial* serial;
    QHash<unsigned int, PtGreyInterface *> cameraInterfaces;
    uint widgetx;
    uint widgety;
    uint numCameras;

public slots:
    void openController(QString);
    void openPGCamera(int);
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
    //void openPtGreyCamera(FlyCapture2::PGRGuid guid);
    void openFakeVideo();
    void countFrames(QImage);

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
    int frameCount;
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
    bool foundController;
    enum CameraState {
        SYNC,
        ASYNC
    };
    CameraState cameraState;
    QSignalMapper* cameraMapper;
    QMenu *cameraMenu;
};

#endif // MAINWINDOW_H
