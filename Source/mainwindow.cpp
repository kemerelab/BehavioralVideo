#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "serialcameracontroller.h"
#include "threads.h"
#include "dummycameracontroller.h"

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
    controllerInitialized = false;
    bool firmware = false;
    bool hardware = false;
    triggerType = NO_SELECTION;

    // Build Controller Menu
    connect(ui->actionDummyController, SIGNAL(triggered()), this,SLOT(openDummyController()));
    if (isSerialControllerConnected()) {
        ui->actionSerialController->setEnabled(true);
        ui->actionSerialController->setChecked(false);
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

    connect(ui->actionFakeVideo, SIGNAL(triggered()), this, SLOT(openFakeVideo()));

    dataController = new(DataController);
    dataController->moveToThread(&dataControllerThread);

    connect(ui->actionOpenVideoFile, SIGNAL(triggered()), this, SLOT(openVideoFile()));
    connect(this, SIGNAL(initializeVideoWriting(QString)), dataController, SLOT(initializeVideoWriting(QString)));
    qRegisterMetaType<SavingState>("SavingState");
    connect(dataController, SIGNAL(updateSavingMenus(SavingState)), this, SLOT(updateVideoMenus(SavingState)));
    connect(ui->actionRecord, SIGNAL(triggered()), dataController, SLOT(startVideoRecording()));
    connect(ui->actionStop, SIGNAL(triggered()), dataController, SLOT(stopVideoWriting()));

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

    QString defaultFilename = QDir::currentPath() + "/BehaviorVideo_" + curtime.toString("MMddyyyy_hhmmss");

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


void MainWindow::updateVideoMenus(SavingState state)
{
    qDebug() << "updating menus " << savingState;
    switch (state) {
        case READY_TO_WRITE:
            qDebug() << "enablevideosaving";
            ui->actionRecord->setEnabled(true);
            ui->actionOpenVideoFile->setDisabled(true);
            savingState = READY_TO_WRITE;
            break;

        case CURRENTLY_WRITING:
            ui->actionRecord->setDisabled(true);
            ui->actionStop->setEnabled(true);
            if (controllerInitialized)
                ui->actionOpenVideoFile->setDisabled(true);
            savingState = CURRENTLY_WRITING;
            break;

        case NOT_SAVING:
            ui->actionRecord->setDisabled(true);
            ui->actionStop->setDisabled(true);
            if (controllerInitialized)
                ui->actionOpenVideoFile->setEnabled(true);
            qDebug() << "menus reset";
            break;
    }
}

void MainWindow::resetSavingMenus()
{
    ui->actionRecord->setDisabled(true);
    ui->actionStop->setDisabled(true);
    if (controllerInitialized)
        ui->actionOpenVideoFile->setEnabled(true);
    qDebug() << "menus reset";
}

void MainWindow::openSerialController()
{
    qDebug()<< "Initializing Controller";
    controller = new SerialCameraController;
    if (((SerialCameraController *)controller)->connect(name) != 0)
    {
        qDebug() << "Error opening controller";
        ui->actionSerialController->setDisabled(true);
        return;
    }

    triggerType = EXTERNAL_CAMERA_TRIGGER;
    openController();
    ui->actionDummyController->setChecked(true);
}

void MainWindow::openDummyController()
{
    controller = new DummyCameraController;
    triggerType = NO_CAMERA_TRIGGER;
    openController();
    ui->actionDummyController->setChecked(true);
}

void MainWindow::openController()
{
    controllerInitialized = true;
    qRegisterMetaType<GenericCameraController*>("GenericCameraController*");

    QMetaObject::invokeMethod(dataController, "registerCameraController", Qt::QueuedConnection,
                              Q_ARG(GenericCameraController*, controller));
    qRegisterMetaType<TriggerType>("TriggerType");
    QMetaObject::invokeMethod(dataController, "useTriggering", Qt::QueuedConnection, Q_ARG(TriggerType, triggerType));
    ui->actionOpenVideoFile->setEnabled(true);
}

void MainWindow::openPGCamera(int serialNumber)
{
    if (!controllerInitialized) {
        QMessageBox::warning(this, "Controller Not Selected", "No Controller Selected");
        return;
    }

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
    if (!controllerInitialized) {
        QMessageBox::warning(this, "Controller Not Selected", "No Controller Selected");
        return;
    }

    FakeVideoGenerator *fakeCamera = new FakeVideoGenerator();
    fakeCamera->cameraName = QString::number(numCameras);
    openCamera(fakeCamera);
}

void MainWindow::openCamera(GenericCameraInterface *camera)
{

    VideoWriter *videoWriter = new VideoWriter();

    if (numCameras == 1){
        camera->moveToThread(&cameraThread1);
        //videoWriter->moveToThread(&videoWriterThread1);
    }
    else{
        camera->moveToThread(&cameraThread0);
        //videoWriter->moveToThread(&videoWriterThread0);
    }

    videoWriter->moveToThread(&videoWriterThread0);

    qRegisterMetaType<GenericCameraInterface*>("GenericCameraInterface*");
    qRegisterMetaType<VideoWriter*>("VideoWriter*");

    QMetaObject::invokeMethod(dataController, "registerCameraAndWriter", Qt::QueuedConnection, Q_ARG(GenericCameraInterface*, camera),
                              Q_ARG(VideoWriter*, videoWriter));

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

    QMetaObject::invokeMethod(dataController, "startVideoViewing", Qt::QueuedConnection);
//    emit startCaptureAsync();
}

void MainWindow::selectPin(QString id){
    qDebug() << "Setting " + id.mid(1) + " to trigger off pin "+ id.left(1);
    PtGreyInterface* tempPGCamera = cameraInterfaces[id.mid(1).toInt()];
    QMetaObject::invokeMethod(tempPGCamera, "ChangeTriggerPin", Qt::QueuedConnection,Q_ARG(int, id.left(1).toInt()));
}


