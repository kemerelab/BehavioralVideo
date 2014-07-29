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
#include <QMenuBar>

#include <QtUiTools/QtUiTools>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    counter = 0;


    ui->setupUi(this);
    layout = new QGridLayout(ui->centralWidget);

    widgetx = 0;
    widgety = 1;
    numcameras = 0;

    //detect serial ports and add to menu
    QSignalMapper* signalMapper = new QSignalMapper (this);
    QMenu *controller = new QMenu("Open Controller");
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
          if (info.manufacturer() != ""){
               qDebug() << "Name        : " << info.portName();
               qDebug() << "Description : " << info.description();
               qDebug() << "Manufacturer: " << info.manufacturer();
               name = info.portName();
               QAction *action = new QAction(info.description(),this);
               controller->addAction(action);
               connect(action,SIGNAL(triggered()),signalMapper,SLOT(map()));
               signalMapper->setMapping(action, info.portName());
           }
    }

    connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(openController(QString)));
    ui->menuFile->insertMenu(ui->actionQuit,controller);



    //detect cameras and add to menu

    QSignalMapper* cameraMapper = new QSignalMapper(this);
    QMenu *cameraMenu = new QMenu("Open Camera");

    FlyCapture2::BusManager busMgr;
    FlyCapture2::PGRGuid guid;
    FlyCapture2::CameraInfo camInfo;
    FlyCapture2::Camera cameralist[10];
    unsigned int x;
    for (x=0; x<10;x++)
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
    connect(ui->actionRecord, SIGNAL(triggered()), this, SLOT(handleVideoSaving()));
    connect(ui->actionStop, SIGNAL(triggered()), this, SLOT(handleVideoSaving()));

    connect(ui->actionFakeVideo, SIGNAL(triggered()), this, SLOT(openFakeVideo()));

    intermediateSavingState = NOT_SAVING;

}

MainWindow::~MainWindow()
{
    delete ui;
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
    if (intermediateSavingState == CURRENTLY_SAVING) {
        frameCount++;
        if (frameCount >= 30 ) {

            //ui->actionStop->trigger();
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
        filename = QFileDialog::getSaveFileName(0,tr("SelectVideoFilename"),QDir::currentPath());
                    //this, tr("Select Video Filename"));
                                                //tr(defaultFilename), tr("Video File (*.mp4)"), 0);
                                                        //QFileDialog::DontUseNativeDialog);

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

    if (fileSelected){
        foreach(uint serialnumber, cameraDictionary.keys()){
            emit cameraDictionary[serialnumber]->initializeVideo(filename + QString::number(serialnumber) + ".mp4");
        }
    }

}

void MainWindow::enableVideoSaving()
{
    qDebug() << "enablevideosaving";
    ui->actionRecord->setEnabled(true);
    ui->actionOpenVideoFile->setDisabled(true);
}

void MainWindow::handleVideoSaving()
{
    // This is where we want to stop the camera, and then restart it
    // with strobes once video saving is started (below)

    qDebug() << intermediateSavingState;
    foreach(PtGreyInterface* camera, cameraDictionary){
        switch (intermediateSavingState) {
        case NOT_SAVING: // should be triggered by menu
            QMetaObject::invokeMethod(camera, "StopCapture", Qt::QueuedConnection);
            intermediateSavingState = STARTING_SAVING;
            break;
        case STARTING_SAVING: // should be triggered by captureEnded
            QMetaObject::invokeMethod(camera->videowriter, "beginWriting", Qt::QueuedConnection);
            QMetaObject::invokeMethod(serial, "startTriggerSync", Qt::QueuedConnection);
            intermediateSavingState = CURRENTLY_SAVING;
            break;
        case CURRENTLY_SAVING: // should be triggered by menu
            QMetaObject::invokeMethod(camera, "StopCapture", Qt::QueuedConnection);
            intermediateSavingState = ENDING_SAVING;
            break;
        case ENDING_SAVING: // should be triggered by captureEnded
            QMetaObject::invokeMethod(camera->videowriter, "endWriting", Qt::QueuedConnection);
            intermediateSavingState = NOT_SAVING;
            QMetaObject::invokeMethod(serial, "stopTrigger", Qt::QueuedConnection);
            break;
        }
    }


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


void MainWindow::openController(QString name)
{

    qDebug()<< "Initializing Controller";
    serial = new Serial;
    serial->connect(name);


}

void MainWindow::openPGCamera(int serialnumber)
{
    PtGreyInterface* pgCamera = new PtGreyInterface();

    if (counter == 1){
        pgCamera->moveToThread(&pgThread1);
    }
    else{
        pgCamera->moveToThread(&pgThread0);
        counter++;
    }
    //pgCamera->moveToThread(&pgThread);

    pgCamera->videowriter = new VideoWriter();
    // Should connect this signal to the writing process so that files get closed
    pgCamera->videowriter->moveToThread(&videoWriterThread);

    connect(pgCamera->videowriter, SIGNAL(videoInitialized()), this, SLOT(enableVideoSaving()));
    connect(pgCamera->videowriter, SIGNAL(writingStarted()), this, SLOT(videoSavingStarted()));
    connect(pgCamera->videowriter, SIGNAL(writingEnded()), this, SLOT(disableVideoSaving()));


    connect(pgCamera,SIGNAL(initializeVideo(QString)),pgCamera->videowriter,SLOT(initialize(QString)));

    cameraDictionary.insert(serialnumber,pgCamera);

    videoWidget[numcameras] = new VideoGLWidget();
    videoWidget[numcameras]->setSurfaceType(QSurface::OpenGLSurface);
    videoWidget[numcameras]->create();
    QWidget *container = QWidget::createWindowContainer(videoWidget[numcameras]);

    if (((float)widgetx*(float)4)/((float)widgety*(float)3) < (float)16/(float)9){
        widgetx++;
    }
    else
    {
        widgety++;
        widgetx = 1;
    }

    layout->addWidget(container,widgety,widgetx);

    QObject::connect(pgCamera, SIGNAL(capturingEnded()), this, SLOT(handleVideoSaving()));
    QObject::connect(pgCamera, SIGNAL(newFrame(QImage)),videoWidget[numcameras],
                     SLOT(newFrame(QImage)));
    QObject::connect(pgCamera, SIGNAL(newFrame(QImage)),pgCamera->videowriter,
                     SLOT(newFrame(QImage)));
    frameCount = 0;
    QObject::connect(pgCamera, SIGNAL(newFrame(QImage)),this,
                     SLOT(countFrames(QImage)));
    QObject::connect(pgCamera->videowriter, SIGNAL(writingStarted()),pgCamera,
                     SLOT(StartCaptureWithStrobe()));
    QObject::connect(pgCamera->videowriter, SIGNAL(writingEnded()),pgCamera,
                     SLOT(StartCaptureNoStrobe()));
    QMetaObject::invokeMethod(pgCamera, "Initialize", Qt::QueuedConnection,Q_ARG(uint, serialnumber));
    numcameras++;

}


