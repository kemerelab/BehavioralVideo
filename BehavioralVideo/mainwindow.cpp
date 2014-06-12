#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fakecamerainterface.h"
#include "threads.h"

#include <QThread>
#include <QTimer>
#include <QDateTime>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include "serial.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    layout = new QGridLayout(ui->centralWidget);

    videoWidget = new VideoGLWidget();

    videoWidget->setSurfaceType(QSurface::OpenGLSurface);
    videoWidget->create();
    QWidget *container = QWidget::createWindowContainer(videoWidget);
    layout->addWidget(container,1,1);
    ui->centralWidget->setLayout(layout);
    //videoWidget->setAnimating(true);

    videoWriter = new VideoWriter();
    // Should connect this signal to the writing process so that files get closed
    videoWriter->moveToThread(&videoWriterThread);

    connect(ui->actionOpenVideoFile, SIGNAL(triggered()), this, SLOT(openVideoFile()));
    connect(this, SIGNAL(initializeVideo(QString)), videoWriter, SLOT(initialize(QString)));
    connect(videoWriter, SIGNAL(videoInitialized()), this, SLOT(enableVideoSaving()));
    connect(ui->actionRecord, SIGNAL(triggered()), this, SLOT(handleVideoSaving()));

    connect(ui->actionStop, SIGNAL(triggered()), this, SLOT(handleVideoSaving()));
    //connect(ui->actionStop, SIGNAL(triggered()), videoWriter, SLOT(endWriting()));

    connect(videoWriter, SIGNAL(writingStarted()), this, SLOT(videoSavingStarted()));
    connect(videoWriter, SIGNAL(writingEnded()), this, SLOT(disableVideoSaving()));
    connect(ui->actionPointGrey, SIGNAL(triggered()), this, SLOT(openPtGreyCamera()));
    connect(ui->actionFakeVideo, SIGNAL(triggered()), this, SLOT(openFakeVideo()));

    intermediateSavingState = NOT_SAVING;
    //qDebug() << "Thread for mainwindow: " << QThread::currentThreadId();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openPtGreyCamera()
{
    pgCamera = new PtGreyInterface();
    pgCamera->moveToThread(&pgThread);
    ui->menuOpen_Camera->setDisabled(true);
    QObject::connect(pgCamera, SIGNAL(capturingEnded()), this, SLOT(handleVideoSaving()));
    QObject::connect(pgCamera, SIGNAL(newFrame(QImage)),videoWidget,
                     SLOT(newFrame(QImage)));
    QObject::connect(pgCamera, SIGNAL(newFrame(QImage)),videoWriter,
                     SLOT(newFrame(QImage)));
    frameCount = 0;
    QObject::connect(pgCamera, SIGNAL(newFrame(QImage)),this,
                     SLOT(countFrames(QImage)));
    QObject::connect(videoWriter, SIGNAL(writingStarted()),pgCamera,
                     SLOT(StartCaptureWithStrobe()));
    QObject::connect(videoWriter, SIGNAL(writingEnded()),pgCamera,
                     SLOT(StartCaptureNoStrobe()));
    QMetaObject::invokeMethod(pgCamera, "Initialize", Qt::QueuedConnection);
}

void MainWindow::openFakeVideo()
{
    fakeCamera = new FakeVideoGenerator();QSerialPortInfo myinfo;

    fakeCamera->moveToThread(&cameraThread);
    ui->menuOpen_Camera->setDisabled(true);
    QObject::connect(fakeCamera, SIGNAL(newFrame(QImage)),videoWidget,
                     SLOT(newFrame(QImage)));
    QObject::connect(fakeCamera, SIGNAL(newFrame(QImage)),videoWriter,
                     SLOT(newFrame(QImage)));
    QMetaObject::invokeMethod(fakeCamera, "StartVideo", Qt::QueuedConnection);
}

void MainWindow::countFrames(QImage)
{
    if (intermediateSavingState == CURRENTLY_SAVING) {
        frameCount++;
        if (frameCount >= 30 ) {
            ui->actionStop->trigger();
        }
    }
}

void MainWindow::openVideoFile()
{
    bool fileSelected = false;
    QString filename;
    QDateTime curtime = QDateTime::currentDateTime();

    QString defaultFilename = QDir::currentPath() + "/BehaviorVideo_" + curtime.toString("MMddyyyy_hhmmss") + ".mp4";

    while (!fileSelected) {
        filename = QFileDialog::getSaveFileName(this, tr("Select Video Filename"),
                                                defaultFilename, tr("Video File (*.mp4)"), 0,
                                                        QFileDialog::DontUseNativeDialog);

        if (filename == NULL)
            break;
        if (QFile(filename).exists()) {
            QMessageBox msgBox;
            msgBox.setText("File exists.");
            msgBox.setInformativeText("Do you want to overwrite?");
            msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
            msgBox.setDefaultButton(QMessageBox::No);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes) {
                fileSelected = true;
            }
        }
        else if (filename != NULL){
            fileSelected = true;
        }
    }

    if (fileSelected)
        emit initializeVideo(filename);
}

void MainWindow::enableVideoSaving()
{
    ui->actionRecord->setEnabled(true);
    ui->actionOpenVideoFile->setDisabled(true);
}

void MainWindow::handleVideoSaving()
{
    // This is where we want to stop the camera, and then restart it
    // with strobes once video saving is started (below)
    switch (intermediateSavingState) {
    case NOT_SAVING: // should be triggered by menu
        QMetaObject::invokeMethod(pgCamera, "StopCapture", Qt::QueuedConnection);
        intermediateSavingState = STARTING_SAVING;
        break;
    case STARTING_SAVING: // should be triggered by captureEnded
        QMetaObject::invokeMethod(videoWriter, "beginWriting", Qt::QueuedConnection);
        intermediateSavingState = CURRENTLY_SAVING;
        break;
    case CURRENTLY_SAVING: // should be triggered by menu
        QMetaObject::invokeMethod(pgCamera, "StopCapture", Qt::QueuedConnection);
        intermediateSavingState = ENDING_SAVING;
        break;
    case ENDING_SAVING: // should be triggered by captureEnded
        QMetaObject::invokeMethod(videoWriter, "endWriting", Qt::QueuedConnection);
        intermediateSavingState = NOT_SAVING;
        break;
    };


}

void MainWindow::videoSavingStarted()
{
    ui->actionRecord->setDisabled(true);
    ui->actionStop->setEnabled(true);
    ui->actionOpenVideoFile->setDisabled(true);
}

void MainWindow::disableVideoSaving()
{
    ui->actionRecord->setDisabled(true);
    ui->actionStop->setDisabled(true);
    ui->actionOpenVideoFile->setEnabled(true);
    qDebug() << "Disable video saving triggered";
}


void MainWindow::on_actionOpen_Serial_triggered()
{
    Serial Ser;
    Ser.openSerial();
}
