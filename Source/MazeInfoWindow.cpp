#include "MazeInfoWindow.h"
#include <QGridLayout>
#include <QDebug>
#include <QButtonGroup>

MazeInfoWindow::MazeInfoWindow(int nWells, QWidget *parent) :
    QDockWidget(parent)
{
    setAllowedAreas(Qt::RightDockWidgetArea);
    setWindowTitle("Maze Controller Info");


    container = new QWidget(this);
    setWidget(container);

    layout = new QGridLayout(container);
    container->setLayout(layout);

    logWindow = new QPlainTextEdit(container);
    logWindow->setReadOnly(true);
    layout->addWidget(logWindow,1,0,5,1);

    numberOfWells = nWells;
    for (int i = 0; i < nWells; i++)
        wellCounts << 0;

    setupWellButtons(nWells);

}

void MazeInfoWindow::setupWellButtons(int nWells)
{
    QWidget *buttonContainer = new QWidget(container);
    layout->addWidget(buttonContainer, 0,0);

    QGridLayout *subLayout = new QGridLayout(buttonContainer);
    buttonContainer->setLayout(subLayout);

    QButtonGroup *wellButtonGroup = new QButtonGroup(this);
    for (int i = 0; i < nWells; i++) {
        QPushButton *button = new QPushButton(this);
        wellButtons.append(button);
        subLayout->addWidget(button,0,2*i);
        button->setText("\n" + QString::number(wellCounts[i]) + "\n");
        button->setStyleSheet("QPushButton {font-size: 18pt;font-weight: bold;}");
    }
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

void MazeInfoWindow::newWellVisitEvent(ulong time, int well)
{
    logWindow->appendPlainText("[" + QString::number(time) + "] " + "Visited well " + QString::number(well));
}

void MazeInfoWindow::newWellRewardEvent(ulong time, int well)
{
    logWindow->appendPlainText("[" + QString::number(time) + "] " + "Rewarded well " + QString::number(well));
}

void MazeInfoWindow::newWellCounts(QList<int> newCounts)
{
    for (int i = 0; i < newCounts.length(); i++) {
        wellCounts[i] = newCounts[i];
        wellButtons.at(i)->setText("\n" + QString::number(wellCounts[i]) + "\n");
    }
}
