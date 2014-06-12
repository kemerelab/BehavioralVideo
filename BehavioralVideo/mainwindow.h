#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGridLayout>
#include "videowidget.h"
#include "videoglwidget.h"
#include "videowriter.h"
#include "ptgreyinterface.h"
#include "fakecamerainterface.h"

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

public slots:
    void openVideoFile();
    void enableVideoSaving();
    void handleVideoSaving();
    void videoSavingStarted();
    void disableVideoSaving();
    void openPtGreyCamera();
    void openFakeVideo();
    void countFrames(QImage);


signals:
    void initializeVideo(QString filename);

private slots:
    void on_actionOpen_Serial_triggered();

private:
    Ui::MainWindow *ui;
    QGridLayout *layout;
    int frameCount;


    enum IntermediateSavingState {
        NOT_SAVING,
        STARTING_SAVING,
        CURRENTLY_SAVING,
        ENDING_SAVING
    };
    IntermediateSavingState intermediateSavingState;
};

#endif // MAINWINDOW_H
