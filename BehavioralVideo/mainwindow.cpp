#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "camerainterface.h"
#include <QThread>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    layout = new QGridLayout(ui->centralWidget);

    videoWidget = new VideoGLWidget();

    //QSurfaceFormat format;
    //format.setSamples(16);
    //vglWidget.setFormat(format);
    //vglWidget.resize(640, 480);
    //vglWidget.show();
    //vglWidget.setAnimating(true);
    videoWidget->setSurfaceType(QSurface::OpenGLSurface);
    videoWidget->create();
    QWidget *container = QWidget::createWindowContainer(videoWidget);
    layout->addWidget(container,1,1);
//    layout->addWidget(videoWidget,1,1);
    ui->centralWidget->setLayout(layout);
    videoWidget->setAnimating(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}
