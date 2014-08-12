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
    foundController = false;
    bool firmware = false;
    bool hardware = false;

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {

          if (info.description().left(17) == "Camera Controller"){

                 foundController = true;

                 //check for correct PCB and Firmware

                 if (info.description().mid(18,7) == "PCB 1.0"){

                     hardware = true;
                 }
                 if (info.description().mid(26,7) == "F W 1.0"){
                     firmware = true;
                 }
                 if (hardware && firmware){
                     openController(info.description());
                 }

                 break;
           }
    }
       if (foundController == false){

           QErrorMessage errorMessage;
           errorMessage.showMessage("Controller not found. Saving disabled");
           ui->actionOpenVideoFile->setDisabled(true);
           errorMessage.exec();
       }
       else if (!hardware && !firmware){
           QErrorMessage errorMessage;
           errorMessage.showMessage("Controller hardware and firmware outdated. Saving disabled");
           ui->actionOpenVideoFile->setDisabled(true);
           errorMessage.exec();
       }
       else if (!hardware){
           QErrorMessage errorMessage;
           errorMessage.showMessage("Controller hardware outdated. Saving disabled");
           ui->actionOpenVideoFile->setDisabled(true);
           errorMessage.exec();
       }
       else if (!firmware){
           QErrorMessage errorMessage;
           errorMessage.showMessage("Controller firmware outdated. Saving disabled");
           ui->actionOpenVideoFile->setDisabled(true);
           errorMessage.exec();
       }


    //detect cameras and add to menu

    cameraMapper = new QSignalMapper(this);
    cameraMenu = new QMenu("Open Camera");

    FlyCapture2::BusManager busMgr;
    FlyCapture2::PGRGuid guid;
    FlyCapture2::CameraInfo camInfo;
    FlyCapture2::Camera cameralist[MAX_CAMERAS];
    unsigned int x;
    for (x=0; x<MAX_CAMERAS;x++)
    {

        if (busMgr.GetCameraFromIndex(x, &guid) != FlyCapture2::PGRERROR_OK)
        {
            break;
        }
        cameralist[x].Connect(&guid);
        cameralist[x].GetCameraInfo(&camInfo);
        QAction *action = new QAction("Point Grey " + QString::number(camInfo.serialNumber),this);
        cameraMenu->addAction(action);
        connect(action, SIGNAL(triggered()),cameraMapper,SLOT(map()));
        cameraMapper->setMapping(action,camInfo.serialNumber);
    }

    connect(cameraMapper,SIGNAL(mapped(int)),this,SLOT(openPGCamera(int)));
    ui->menuFile->insertMenu(ui->actionQuit,cameraMenu);

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

void MainWindow::openController(QString name)
{

    qDebug()<< "Initializing Controller";
    serial = new Serial;
    serial->connect(name);

    connect(this, SIGNAL(videoCaptureStartedSync()), serial, SLOT(startTriggerNoSync()));
    connect(serial, SIGNAL(triggersStarted(bool)), this, SLOT(updateVideoSavingMenus(bool)));

    connect(ui->actionRecord, SIGNAL(triggered()), serial, SLOT(stopTrigger()));
    connect(ui->actionStop, SIGNAL(triggered()), serial, SLOT(stopTrigger()));
    connect(serial, SIGNAL(triggersStopped()), this, SLOT(controlVideoWriter()));
    // video writing signals get aggregated into videoWritingStarted - this is our signal to start triggers again
    connect(this, SIGNAL(videoWritingStarted()), serial, SLOT(startTriggerSync()));


}

void MainWindow::openPGCamera(int serialnumber)
{
    bool initialized = false;
    foreach (int initializedSerial, cameraInterfaces.keys()){
        if (initializedSerial == serialnumber){
            initialized = true;
        }
    }

    //remove menu item and replace with disabled dummy
    if (initialized == false){
        cameraMenu->removeAction(((QAction *)cameraMapper->mapping(serialnumber)));
        QAction *tempaction = new QAction("Point Grey " + QString::number(serialnumber),this);
        tempaction->setDisabled(true);
        cameraMenu->addAction(tempaction);


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


        PtGreyInterface* pgCamera = new PtGreyInterface();
        pgCamera->serialNumber = serialnumber;

        if (numCameras == 1){
            pgCamera->moveToThread(&pgThread1);
        }
        else{
            pgCamera->moveToThread(&pgThread0);
        }


        pgCamera->videowriter = new VideoWriter();
        pgCamera->videowriter->moveToThread(&videoWriterThread);

        connect(pgCamera, SIGNAL(capturingStarted()), this, SLOT(aggregateVideoCaptureStarted()));
        connect(pgCamera, SIGNAL(capturingEnded()), this, SLOT(aggregateVideoCaptureEnded()));
        connect(pgCamera->videowriter, SIGNAL(writingEnded()), this, SLOT(aggregateVideoWritingEnded()));
        connect(pgCamera->videowriter, SIGNAL(videoInitialized()), this, SLOT(aggregateVideoWritingInitialized()));
        connect(pgCamera->videowriter, SIGNAL(writingStarted()), this, SLOT(aggregateVideoWritingStarted()));

        connect(this,SIGNAL(initializeVideoWriting(QString)),pgCamera,SLOT(InitializeVideoWriting(QString)));
        connect(pgCamera,SIGNAL(initializeVideoWriting(QString)),pgCamera->videowriter,SLOT(initialize(QString)));
        connect(this, SIGNAL(startCaptureAsyncSignal()), pgCamera, SLOT(StartCameraCaptureAsync()));
        connect(this, SIGNAL(startCaptureSyncSignal()), pgCamera, SLOT(StartCameraCaptureSync()));
        connect(this, SIGNAL(restartCaptureAsyncSignal()), pgCamera, SLOT(StopAndRestartCaptureAsync()));
        connect(this, SIGNAL(restartCaptureSyncSignal()), pgCamera, SLOT(StopAndRestartCaptureSync()));
        connect(this, SIGNAL(startVideoWriting()), pgCamera->videowriter, SLOT(beginWriting()));
        connect(this, SIGNAL(endVideoWriting()), pgCamera->videowriter, SLOT(endWriting()));

        cameraInterfaces.insert(serialnumber,pgCamera);

        videoWidget[numCameras] = new VideoGLWidget();
        videoWidget[numCameras]->setSurfaceType(QSurface::OpenGLSurface);
        videoWidget[numCameras]->create();
        QWidget *container = QWidget::createWindowContainer(videoWidget[numCameras]);

        if (((float)widgetx*(float)4)/((float)widgety*(float)3) < (float)16/(float)9){
            widgetx++;
        }
        else
        {
            widgety++;
            widgetx = 1;
        }

        layout->addWidget(container,widgety,widgetx);

        QObject::connect(pgCamera, SIGNAL(newFrame(QImage)),videoWidget[numCameras],
                         SLOT(newFrame(QImage)));
        QObject::connect(pgCamera, SIGNAL(newFrame(QImage)),pgCamera->videowriter,
                         SLOT(newFrame(QImage)));
        frameCount = 0;
        QObject::connect(pgCamera, SIGNAL(newFrame(QImage)),this,
                         SLOT(countFrames(QImage)));
        QMetaObject::invokeMethod(pgCamera, "Initialize", Qt::QueuedConnection,Q_ARG(uint, serialnumber));

        numCameras++;

        emit startCaptureAsync();
    }
}

void MainWindow::selectPin(QString id){
    qDebug() << "Setting " + id.mid(1) + " to trigger off pin "+ id.left(1);
    PtGreyInterface* tempPGCamera = cameraInterfaces[id.mid(1).toInt()];
    QMetaObject::invokeMethod(tempPGCamera, "ChangeTriggerPin", Qt::QueuedConnection,Q_ARG(int, id.left(1).toInt()));
}

//void MainWindow::openPtGreyCamera(){};
void MainWindow::openFakeVideo()
{
    /*
    fakeCamera = new FakeVideoGenerator();

    fakeCamera->moveToThread(&cameraThread);
    //ui->menuOpen_Camera->setDisabled(true);
    QObject::connect(fakeCamera, SIGNAL(newFrame(QImage)),videoWidget,
                     SLOT(newFrame(QImage)));
    QObject::connect(fakeCamera, SIGNAL(newFrame(QImage)),videoWriter,
                     SLOT(newFrame(QImage)));
    QMetaObject::invokeMethod(fakeCamera, "StartVideo", Qt::QueuedConnection);
    */
}

void MainWindow::countFrames(QImage)
{
    if (savingState == CURRENTLY_WRITING) {
        frameCount++;
        if (frameCount >= 30 ) {

            //ui->actionStop->trigger();
        }
    }
}

