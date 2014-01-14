#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "videowidget.h"
#include "camerainterface.h"
#include <QThread>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    layout = new QGridLayout(ui->centralWidget);

    VideoWidget *videoWidget = new VideoWidget(this);
    layout->addWidget(videoWidget,1,1);
    ui->centralWidget->setLayout(layout);

}

MainWindow::~MainWindow()
{
    delete ui;
}
