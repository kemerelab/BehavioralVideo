#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "camerainterface.h"
#include <QThread>
#include <QTimer>
#include <QDateTime>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>

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
    videoWidget->setAnimating(true);

    videoWriter = new VideoWriter();

    connect(ui->actionOpenVideoFile, SIGNAL(triggered()), this, SLOT(openVideoFile()));
    connect(this, SIGNAL(initializeVideo(QString)), videoWriter, SLOT(initialize(QString)));
    connect(videoWriter, SIGNAL(videoInitialized()), this, SLOT(enableVideoSaving()));
    connect(ui->actionRecord, SIGNAL(triggered()), videoWriter, SLOT(beginWriting()));
    connect(ui->actionStop, SIGNAL(triggered()), videoWriter, SLOT(endWriting()));
    connect(videoWriter, SIGNAL(writingEnded()), this, SLOT(disableVideoSaving()));
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
        filename = QFileDialog::getSaveFileName(this,
                                                        tr("Select Video Filename"), defaultFilename, tr("Video File (*.mp4)"), 0,
                                                        QFileDialog::DontUseNativeDialog);

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
    ui->actionStop->setEnabled(true);
}

void MainWindow::disableVideoSaving()
{
    ui->actionRecord->setDisabled(true);
    ui->actionStop->setDisabled(true);
}

