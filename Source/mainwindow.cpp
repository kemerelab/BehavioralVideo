#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fakecamerainterface.h"
#include "threads.h"
#include "ptgreyinterface.h"

#include <QThread>
#include <QTimer>
#include <QDateTime>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <QMenuBar>
#include <QErrorMessage>

#include <QtUiTools/QtUiTools>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    layout = new QGridLayout(ui->centralWidget);

    widgetx = 0;
    widgety = 1;
    numCameras = 0;
    numCamerasReadyToWrite = 0;
    numCamerasInitialized = 0;
    numCamerasCapturing = 0;
    dummyControllerSelected = false;
    bool firmware = false;
    bool hardware = false;

    // Build Controller Menu
    connect(ui->actionDummyController, SIGNAL(triggered()), this,SLOT(openDummyController()));
    if (isSerialControllerConnected()) {
        ui->actionSerialController->setEnabled(true);
        connect(ui->actionSerialController, SIGNAL(triggered()),this,SLOT(openSerialController()));
    }

    connect(ui->actionFakeCamera,SIGNAL(triggered()),this,SLOT(openFakeVideo()));

    //detect Point Grey cameras and add to menu
    QStringList *cameraNameList = new QStringList;
    FindPointGreyCameras(cameraNameList);
    qDebug() << "Number of cameras found " << cameraNameList->size();
    if (cameraNameList->size() > 0) {
        ui->menuOpenCamera->addSection("PointGrey Cameras");
        cameraMapper = new QSignalMapper(this);
        for (int x=0; x < cameraNameList->size(); x++)
        {
            QAction *action = new QAction("Point Grey " + cameraNameList->at(x),this);
            ui->menuOpenCamera->addAction(action);
            connect(action, SIGNAL(triggered()),cameraMapper,SLOT(map()));
            cameraMapper->setMapping(action,cameraNameList->at(x).toUInt());
        }
        connect(cameraMapper,SIGNAL(mapped(int)),this,SLOT(openPGCamera(int)));
    }
    else {
        ui->menuOpenCamera->addSection("No PointGrey Cameras Found");
    }

    ui->centralWidget->setLayout(layout);

    connect(ui->actionOpenVideoFile, SIGNAL(triggered()), this, SLOT(openVideoFile()));
    // openVideo triggers initializeWriting, whose resposes get aggregated into videoWritingInitialized
    connect(this, SIGNAL(videoWritingInitialized()), this, SLOT(restartCaptureSync()));
    // video capture gets aggregated into videoCaptureStarted - this is our signal to start capturing with the triggers
    //   note that the TEE'ing of the triggers will happen later
  //  connect(this, SIGNAL(videoCaptureStartedSync()), serial, SLOT(startTriggerNoSync()));
  //  connect(serial, SIGNAL(triggersStarted(bool)), this, SLOT(updateVideoSavingMenus(bool)));

//    connect(ui->actionRecord, SIGNAL(triggered()), serial, SLOT(stopTrigger()));
//    connect(ui->actionStop, SIGNAL(triggered()), serial, SLOT(stopTrigger()));
//    connect(serial, SIGNAL(triggersStopped()), this, SLOT(controlVideoWriter()));
    // video writing signals get aggregated into videoWritingStarted - this is our signal to start triggers again
//    connect(this, SIGNAL(videoWritingStarted()), serial, SLOT(startTriggerSync()));

    // video writing ended signals get aggregated into videoWritingEnded - this is our signal to go back to the beginning
    connect(this, SIGNAL(videoWritingEnded()), this, SLOT(restartCaptureAsync()));
    connect(this, SIGNAL(videoCaptureStartedAsync()), this, SLOT(resetSavingMenus()));

    connect(ui->actionFakeVideo, SIGNAL(triggered()), this, SLOT(openFakeVideo()));

    savingState = NOT_SAVING;
    cameraState = ASYNC;
}

MainWindow::~MainWindow()
{
    delete ui;
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

    emit initializeVideoWriting(filename);
}


void MainWindow::updateVideoSavingMenus(bool writing)
{
    qDebug() << "updating menus " << savingState;
    if (!writing) {
        qDebug() << "enablevideosaving";
        ui->actionRecord->setEnabled(true);
        ui->actionOpenVideoFile->setDisabled(true);
        savingState = READY_TO_WRITE;
    }
    else {
        ui->actionRecord->setDisabled(true);
        ui->actionStop->setEnabled(true);
        ui->actionOpenVideoFile->setDisabled(true);
        savingState = CURRENTLY_WRITING;
    }
}

void MainWindow::resetSavingMenus()
{
    ui->actionRecord->setDisabled(true);
    ui->actionStop->setDisabled(true);
    ui->actionOpenVideoFile->setEnabled(true);
    qDebug() << "menus reset";
}

void MainWindow::controlVideoWriter()
{
    if (savingState == READY_TO_WRITE) {
        emit startVideoWriting();
    }
    else if (savingState == CURRENTLY_WRITING) {
        qDebug() << "Ending writing";
        emit endVideoWriting();
        savingState = NOT_SAVING;
    }
}

void MainWindow::startCaptureAsync()
{
    cameraState = ASYNC;
    emit startCaptureAsyncSignal();
}

void MainWindow::startCaptureSync()
{
    cameraState = SYNC;
    emit startCaptureSyncSignal();
}

void MainWindow::restartCaptureAsync()
{
    cameraState = ASYNC;
    emit restartCaptureAsyncSignal();
}

void MainWindow::restartCaptureSync()
{
    cameraState = SYNC;
    qDebug() << "restart capture sync";
    emit restartCaptureSyncSignal();
}

void MainWindow::aggregateVideoWritingInitialized()
{
    qDebug() << "aggregating initialize";
    numCamerasInitialized++;
    if (numCamerasInitialized == numCameras) {
        emit videoWritingInitialized();
        qDebug() << "emitted video writing initialized signal";
    }
}

void MainWindow::aggregateVideoWritingStarted()
{
    numCamerasReadyToWrite++;
    if (numCamerasReadyToWrite == numCameras) {
        emit videoWritingStarted();
    }
}

void MainWindow::aggregateVideoWritingEnded()
{
    numCamerasReadyToWrite--;
    numCamerasInitialized--;
    if (numCamerasReadyToWrite == 0) {
        emit videoWritingEnded();
    }
}

void MainWindow::aggregateVideoCaptureStarted()
{
    qDebug() << "Aggregate video capture started";
    numCamerasCapturing++;
    if (numCamerasCapturing == numCameras) {
        if (cameraState == SYNC)
            emit videoCaptureStartedSync();
        else if (cameraState == ASYNC)
            emit videoCaptureStartedAsync();
    }
}

void MainWindow::aggregateVideoCaptureEnded()
{
    qDebug() << "Aggregate video capture ended";
    numCamerasCapturing--;
    if (numCamerasCapturing == 0) {
        emit videoCaptureEnded();
    }
}

void MainWindow::openSerialController()
{

    qDebug()<< "Initializing Controller";
    serialController = new SerialCameraController;
    if (serialController->connect(name) != 0)
    {
        qDebug() << "Error opening controller";
        ui->actionSerialController->setDisabled(true);
        return;
    }
    connect(this, SIGNAL(videoCaptureStartedSync()), serialController, SLOT(startTriggerNoSync()));
    connect(serialController, SIGNAL(triggersStarted(bool)), this, SLOT(updateVideoSavingMenus(bool)));

    connect(ui->actionRecord, SIGNAL(triggered()), serialController, SLOT(stopTrigger()));
    connect(ui->actionStop, SIGNAL(triggered()), serialController, SLOT(stopTrigger()));
    connect(serialController, SIGNAL(triggersStopped()), this, SLOT(controlVideoWriter()));
    // video writing signals get aggregated into videoWritingStarted - this is our signal to start triggers again
    connect(this, SIGNAL(videoWritingStarted()), serialController, SLOT(startTriggerSync()));
}

void MainWindow::openDummyController()
{
    dummyController = new DummyCameraController;
    connect(this, SIGNAL(videoCaptureStartedSync()), dummyController, SLOT(startTriggerNoSync()));
    connect(dummyController, SIGNAL(triggersStarted(bool)), this, SLOT(updateVideoSavingMenus(bool)));

    connect(ui->actionRecord, SIGNAL(triggered()), dummyController, SLOT(stopTrigger()));
    connect(ui->actionStop, SIGNAL(triggered()), dummyController, SLOT(stopTrigger()));
    connect(dummyController, SIGNAL(triggersStopped()), this, SLOT(controlVideoWriter()));
    // video writing signals get aggregated into videoWritingStarted - this is our signal to start triggers again
    connect(this, SIGNAL(videoWritingStarted()), dummyController, SLOT(startTriggerSync()));

}

void MainWindow::openPGCamera(int serialNumber)
{
    /*
        //add camera to camer menu

        ui->menuCamera->addAction((QAction *)cameraMapper->mapping(serialnumber));

        //add settings to camera menu
        QMenu *settingMenu = new QMenu();
        QMenu *triggerMenu = new QMenu("Trigger Pin");
        QAction *pinAction0 = new QAction("Pin 0",this);
        QAction *pinAction1 = new QAction("Pin 1",this);
        QAction *pinAction2 = new QAction("Pin 2",this);
        QAction *pinAction3 = new QAction("Pin 3",this);
        triggerMenu->addAction(pinAction0);
        triggerMenu->addAction(pinAction1);
        triggerMenu->addAction(pinAction2);
        triggerMenu->addAction(pinAction3);
        settingMenu->addMenu(triggerMenu);
        ((QAction *)cameraMapper->mapping(serialnumber))->setMenu(settingMenu);
        QSignalMapper* pinMapper =  new QSignalMapper(this);
        pinMapper->setMapping(pinAction0,"0" + QString::number(serialnumber));
        pinMapper->setMapping(pinAction1,"1" + QString::number(serialnumber));
        pinMapper->setMapping(pinAction2,"2" + QString::number(serialnumber));
        pinMapper->setMapping(pinAction3,"3" + QString::number(serialnumber));
        connect(pinAction0, SIGNAL(triggered()),pinMapper,SLOT(map()));
        connect(pinAction1, SIGNAL(triggered()),pinMapper,SLOT(map()));
        connect(pinAction2, SIGNAL(triggered()),pinMapper,SLOT(map()));
        connect(pinAction3, SIGNAL(triggered()),pinMapper,SLOT(map()));
        connect(pinMapper,SIGNAL(mapped(QString)),this,SLOT(selectPin(QString)));
        */

    PtGreyInterface* pgCamera = new PtGreyInterface();
    pgCamera->serialNumber = serialNumber;
    openCamera(pgCamera);
    ((QAction *)cameraMapper->mapping(serialNumber))->setDisabled(true);
}

void MainWindow::openFakeVideo()
{
    FakeVideoGenerator *fakeCamera = new FakeVideoGenerator();
    openCamera(fakeCamera);
}

void MainWindow::openCamera(GenericCameraInterface *camera)
{

    VideoWriter *videoWriter = new VideoWriter();

    if (numCameras == 1){
        camera->moveToThread(&cameraThread1);
        videoWriter->moveToThread(&videoWriterThread1);
    }
    else{
        camera->moveToThread(&cameraThread0);
        videoWriter->moveToThread(&videoWriterThread0);
    }

    connect(camera, SIGNAL(capturingStarted()), this, SLOT(aggregateVideoCaptureStarted()));
    connect(camera, SIGNAL(capturingEnded()), this, SLOT(aggregateVideoCaptureEnded()));
    connect(videoWriter, SIGNAL(writingEnded()), this, SLOT(aggregateVideoWritingEnded()));
    connect(videoWriter, SIGNAL(videoInitialized()), this, SLOT(aggregateVideoWritingInitialized()));
    connect(videoWriter, SIGNAL(writingStarted()), this, SLOT(aggregateVideoWritingStarted()));

    connect(this,SIGNAL(initializeVideoWriting(QString)),camera,SLOT(InitializeVideoWriting(QString)));
    connect(camera,SIGNAL(initializeVideoWriting(QString)),videoWriter,SLOT(initialize(QString)));
    connect(this, SIGNAL(startCaptureAsyncSignal()), camera, SLOT(StartCameraCaptureAsync()));
    connect(this, SIGNAL(startCaptureSyncSignal()), camera, SLOT(StartCameraCaptureSync()));
    connect(this, SIGNAL(restartCaptureAsyncSignal()), camera, SLOT(StopAndRestartCaptureAsync()));
    connect(this, SIGNAL(restartCaptureSyncSignal()), camera, SLOT(StopAndRestartCaptureSync()));
    connect(this, SIGNAL(startVideoWriting()), videoWriter, SLOT(beginWriting()));
    connect(this, SIGNAL(endVideoWriting()), videoWriter, SLOT(endWriting()));

    VideoGLWidget *videoWidget = new VideoGLWidget();
    videoWidget->setSurfaceType(QSurface::OpenGLSurface);
    videoWidget->create();
    QWidget *container = QWidget::createWindowContainer(videoWidget);

    if (((float)widgetx*(float)4)/((float)widgety*(float)3) < (float)16/(float)9){
        widgetx++;
    }
    else
    {
        widgety++;
        widgetx = 1;
    }

    layout->addWidget(container,widgety,widgetx);

    QObject::connect(camera, SIGNAL(newFrame(QImage)),videoWidget, SLOT(newFrame(QImage)));
    QObject::connect(camera, SIGNAL(newFrame(QImage)),videoWriter, SLOT(newFrame(QImage)));

    //cameraInterfaces.insert(serialNumber,pgCamera);
    QMetaObject::invokeMethod(camera, "Initialize", Qt::QueuedConnection);

    numCameras++;

    emit startCaptureAsync();
}

void MainWindow::selectPin(QString id){
    qDebug() << "Setting " + id.mid(1) + " to trigger off pin "+ id.left(1);
    PtGreyInterface* tempPGCamera = cameraInterfaces[id.mid(1).toInt()];
    QMetaObject::invokeMethod(tempPGCamera, "ChangeTriggerPin", Qt::QueuedConnection,Q_ARG(int, id.left(1).toInt()));
}


