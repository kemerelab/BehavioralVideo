#include "MazeInfoWindow.h"
#include <QGridLayout>
#include <QDebug>

MazeInfoWindow::MazeInfoWindow(QWidget *parent) :
    QDockWidget(parent)
{
    setAllowedAreas(Qt::RightDockWidgetArea);
    setWindowTitle("Maze Controller Info");


    QWidget *container = new QWidget(this);
    setWidget(container);

    QGridLayout *layout = new QGridLayout(container);
    container->setLayout(layout);

    logWindow = new QPlainTextEdit(container);
    logWindow->setReadOnly(true);
    layout->addWidget(logWindow,0,0);

}

void MazeInfoWindow::newLogText(QByteArray text)
{
  logWindow->appendPlainText(QString(text));
}

void MazeInfoWindow::newStatusMessage(QString text)
{
    logWindow->appendPlainText(QString(text));
}

void MazeInfoWindow::newFrameTimestampEvent(ulong time, ulong count)
{
    static int counter = 0;
    if (counter++ == 29) {
        counter = 0;
        logWindow->appendPlainText(QString::number(time) + " [30 triggers received.] " + QString::number(count));
    }
}

void MazeInfoWindow::newWellVisitEvent(ulong time, char well)
{
    logWindow->appendPlainText("[" + QString::number(time) + "] " + "Visited well " + QString(well));
}

void MazeInfoWindow::newWellRewardEvent(ulong time, char well, int count, int total)
{
    logWindow->appendPlainText("[" + QString::number(time) + "] " + "Rewarded well " + QString(well) +
                               " (" + QString::number(count) + ", " + QString::number(total));
}
