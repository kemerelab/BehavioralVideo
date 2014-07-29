#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
    QHash<unsigned int, PtGreyInterface *> cameraDictionary;
    uint widgetx;
    uint widgety;
    uint numcameras;
    uint counter;

public slots:
    void openVideoFile();
    void enableVideoSaving();
    void handleVideoSaving();
    void videoSavingStarted();
    void disableVideoSaving();
    //void openPtGreyCamera(FlyCapture2::PGRGuid guid);
    void openFakeVideo();
    void countFrames(QImage);
    void openController(QString);
    void openPGCamera(int);




signals:

    void initializeController(QString portname);


private slots:


private:
    Ui::MainWindow *ui;
    QGridLayout *layout;
    int frameCount;
    QString name;


    enum IntermediateSavingState {
        NOT_SAVING,
        STARTING_SAVING,
        CURRENTLY_SAVING,
        ENDING_SAVING
    };
    IntermediateSavingState intermediateSavingState;
};

#endif // MAINWINDOW_H
