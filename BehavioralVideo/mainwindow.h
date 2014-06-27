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
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    VideoGLWidget *videoWidget;
    //VideoWidget *videoWidget;
    VideoWriter *videoWriter;
    PtGreyInterface *pgCamera;
    FakeVideoGenerator *fakeCamera;
    Serial serial;

public slots:
    void openVideoFile();
    void enableVideoSaving();
    void handleVideoSaving();
    void videoSavingStarted();
    void disableVideoSaving();
    void openPtGreyCamera();
    void openFakeVideo();
    void countFrames(QImage);
    void openController(QString);
    void openPGCamera(QString);


signals:
    void initializeVideo(QString filename);
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
