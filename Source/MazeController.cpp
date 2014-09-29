#include "MazeController.h"
#include <QDebug>
#include <QThread>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>

bool isMazeControllerConnected()
{
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
          if (info.hasProductIdentifier() && info.hasVendorIdentifier()){
                 if ((info.vendorIdentifier() == 0x2341) && (info.productIdentifier() == 0x0043)){
                     qDebug() << "Found arduino camera controller " << info.description();
                     return true;
                 }
           }
    }
    return false;
}


int MazeController::connectToPort(QString portname)
{
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        if (info.hasProductIdentifier() && info.hasVendorIdentifier()){
            if ((info.vendorIdentifier() == 0x2341) && (info.productIdentifier() == 0x0043)){
                port = new QSerialPort(info);
                port->open(QIODevice::ReadWrite);
                if (!port->isOpen()){
                    qDebug() << "Port not open";
                    return 1;
                }
                port->setBaudRate(QSerialPort::Baud115200);
                port->clear();
                port->flush();
                QThread::msleep(500);
                port->clear();
                port->write("9;"); //ask for version
                port->flush();
                connect(port, SIGNAL(readyRead()), this, SLOT(processSerialData()));
                qDebug() << "Connected to arduino maze controller.";
                return 0;
            }
        }
    }

    qDebug() << "No serial controller found";
    return 2;
}

void MazeController::stopTrigger()
{
    port->write("5,0;"); // disable triggers
    port->flush();
    QThread::msleep(500);
    qDebug() << "Maze controller stop";
}

void MazeController::processSerialData()
{
    int idx;
    QList<QByteArray> chunkedCommands;
    QList<QByteArray> parsedCommand;
    QByteArray command;
    static int counter = 0;
    ulong timestamp;
    char eventType;

    while (!port->atEnd()) {
        QByteArray data;
        data = port->read(port->bytesAvailable());
        if (loggingEnabled)
            logFile->write(data);

        commandBuffer.append(data);
        chunkedCommands = commandBuffer.simplified().split(';'); // parse into commands
        if (!chunkedCommands.isEmpty()) {
            if (!chunkedCommands.last().isEmpty()) {  // partial command at end
                commandBuffer = chunkedCommands.takeLast();
            }
            else {
                chunkedCommands.removeLast(); // get rid of empty string
                commandBuffer = "";
            }
            for (int i = 0; i < chunkedCommands.length(); i++) {
                parsedCommand = chunkedCommands.at(i).split(','); // parse out fields
                if (!parsedCommand.isEmpty()) {
                    switch(parsedCommand.at(0).toInt()) {
                    case (0): // status message
                        emit statusMessageReceived(parsedCommand.at(1));
                        qDebug() << "maze controller status: " <<  parsedCommand.at(1);
                        break;
                    case (1): // event
                        timestamp = parsedCommand.at(1).toULong();
                        eventType = parsedCommand.at(2).at(0);
                        if (eventType == 'T') { // trigger
                            emit frameTimestampEvent(timestamp,parsedCommand.at(3).toLong());
                        }
                        else if (eventType == 'W') {
                            emit wellVisitEvent(timestamp,parsedCommand.at(3).at(0));
                        }
                        else if (eventType == 'R') {
                            emit wellRewardEvent(timestamp,parsedCommand.at(3).at(0),
                                                 parsedCommand.at(4).toInt(),
                                                 parsedCommand.at(5).toInt());
                        }
                        break;
                    case (9):
                        versionString = parsedCommand.at(1);
                        emit statusMessageReceived(versionString);
                        qDebug() << "maze controller version:" << versionString;
                        break;
                    default:
                        qDebug() << "Unexpected command received." << chunkedCommands.at(i);
                    }

                }
            }
        }
    }
}

void MazeController::initializeLogFile(QString filename)
{
    filename = filename + QString(".dat");
    qDebug() << "Initializing writing to " << filename;

   logFile = new QFile(filename);
    if (!logFile->open(QIODevice::WriteOnly)) {
        qDebug() << "Error opening log file for maze controller";
        return;
    }
    // write file header
    logFile->write("# Maze Controller Log File\n");
    logFile->write("# (timestamps are in microseconds)\n");
}

void MazeController::beginWriting()
{
    loggingEnabled = true;
}

void MazeController::endWriting()
{
    loggingEnabled = false;
    logFile->close();
}

void MazeController::changeCameraFrameRate(int)
{

}

void MazeController::changeCameraTriggerPin(int)
{

}

void MazeController::testWell(int)
{

}

void MazeController::resetWellCounts()
{

}

void MazeController::startTrigger(bool syncState) {
    if (syncState) {
        port->write("6,1;"); // enable trigger logging
        port->flush();
        QThread::msleep(500);
        port->write("5,1;"); // enable triggers
        port->flush();
        qDebug() << "triggers started true";
    }
    else {
        port->write("6,0;"); // disable trigger logging
        port->flush();
        QThread::msleep(500);
        port->write("5,1;"); // enable triggers
        port->flush();
        qDebug() << "triggers started false";
    }
}

MazeController::MazeController(QObject *parent) :
    GenericCameraController(parent)
{
    loggingEnabled = false;
    commandBuffer = "";
    availablePins << "A0" << "A1" << "A2" << "A3" << "A4" << "A5" << "6" << "7";
    numberOfWells = 0;
}

MazeController::~MazeController()
{
    port->close();
}


MazeControllerSettingsWidget::MazeControllerSettingsWidget(MazeController *controller, QWidget *parent) :
    QWidget(parent)
{
    QFormLayout *layout = new QFormLayout(this);
    setLayout(layout);

    QLineEdit *infoEdit = new QLineEdit(controller->versionString, this);
    infoEdit->setReadOnly(true);
    layout->addRow(tr("Controller Version"),infoEdit);

    //QComboBox *beamBreakA = new QComboBox(this);
    //beamBreakA->addItem(controller->availablePins);
    //layout->addRow(tr("Pin for beam break A"),beamBreakA);
}

MazeControllerSettingsWidget::~MazeControllerSettingsWidget()
{

}

void MazeControllerSettingsWidget::changeCameraFrameRate(QString)
{

}

void MazeControllerSettingsWidget::changeCameraTriggerPin(int)
{

}
