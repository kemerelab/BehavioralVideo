#include "MazeController.h"
#include <QDebug>
#include <QThread>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>

QStringList listSerialPorts()
{
    QStringList SerialPortNames;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        SerialPortNames << QString(info.portName());
        /*
          if (info.hasProductIdentifier() && info.hasVendorIdentifier()){
                 if ((info.vendorIdentifier() == 0x2341) && (info.productIdentifier() == 0x0043)){
                     qDebug() << "Found arduino camera controller " << info.description();
                     return true;
                 }
          }
          */
    }
    return SerialPortNames;
}

MazeController::MazeController(QObject *parent) :
    GenericCameraController(parent)
{
    availablePins << "A0" << "A1" << "A2" << "A3" << "A4" << "A5" << "6" << "7";
    numberOfWells = 0;

    serialPortInterface = new MazeControllerSerialPort(&condition, 0);
    portThread = new QThread();
    serialPortInterface->moveToThread(portThread);
    portThread->start();
    connect(serialPortInterface, SIGNAL(versionString(QString)), this, SLOT(setVersionString(QString)), Qt::DirectConnection);
    connect(serialPortInterface, SIGNAL(numberOfWells(int)), this, SLOT(setNumberOfWells(int)), Qt::DirectConnection);
    connect(serialPortInterface, SIGNAL(currentPins(QList<int>)), this, SLOT(setPins(QList<int>)), Qt::DirectConnection);
}

MazeController::~MazeController()
{
    delete portThread;
    delete serialPortInterface;
}

int MazeController::connectToPort(QString portname)
{
    int error;
    QMetaObject::invokeMethod(serialPortInterface, "initialize", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(int, error),
                              Q_ARG(QString, portname));
    qDebug() << "Serial port initialize returns " << error;
    if (error == 0) {
        qRegisterMetaType<CommandEnum*>("CommandEnum");
        QMetaObject::invokeMethod(serialPortInterface, "expectResponse", Qt::QueuedConnection,
                                  Q_ARG(CommandEnum, kVersion));
        mutex.lock(); //
        QMetaObject::invokeMethod(serialPortInterface, "sendCommand", Qt::QueuedConnection,
                                  Q_ARG(CommandEnum, kVersion));
        if(condition.wait(&mutex,5000)) { // wait for 500 milliseconds for response
            mutex.unlock();
            qDebug() << "Connected to arduino maze controller." << versionString;
        }
        else {
            qDebug() << "Failed to receive appropriate response to version request.";
            return 2;
        }
        QMetaObject::invokeMethod(serialPortInterface, "expectResponse", Qt::QueuedConnection,
                                  Q_ARG(CommandEnum, kQueryPins));
        mutex.lock(); //
        QMetaObject::invokeMethod(serialPortInterface, "sendCommand", Qt::QueuedConnection,
                                  Q_ARG(CommandEnum, kQueryPins));
        if(condition.wait(&mutex,5000)) { // wait for 500 milliseconds for response
            mutex.unlock();
            qDebug() << "Number of wells: " << numberOfWells;
        }
        return 0;
    }
    else
        return error;
}

void MazeController::stopTrigger()
{
    QMetaObject::invokeMethod(serialPortInterface, "sendCommand", Qt::QueuedConnection,
                              Q_ARG(CommandEnum, kEnableTrigger), Q_ARG(int,0));
    QThread::msleep(500);
    qDebug() << "Maze controller stop";
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
    serialPortInterface->logFile = logFile;

}

void MazeController::beginWriting()
{
    QMetaObject::invokeMethod(serialPortInterface, "enableLogging", Qt::QueuedConnection);
}

void MazeController::endWriting()
{
    QMetaObject::invokeMethod(serialPortInterface, "disableLogging", Qt::QueuedConnection);
    logFile->close();
}

void MazeController::changeCameraFrameRate(int)
{

}

void MazeController::changeCameraTriggerPin(int)
{

}

void MazeController::setPins(QList<int> pinList)
{
    int n = pinList.length();
    if (n != numberOfWells*2 + 2) {
        qDebug() << "Unexpected number of pins in list";
    }
    else {
        beamBreakPins.clear();
        pumpPins.clear();
        qDebug() << "Pin list " << pinList;
        for (int i = 0; i < (n-1); i+=2) {
            beamBreakPins.append(pinList[i]);
            pumpPins.append(pinList[i+1]);
        }
        cameraPin = pinList[n-1];
        cameraLoggingPin = pinList[n-1];
    }
}

void MazeController::testWell(int)
{

}

void MazeController::resetWellCounts()
{

}

void MazeController::startTrigger(bool syncState) {
    if (syncState) {
        QMetaObject::invokeMethod(serialPortInterface, "sendCommand", Qt::QueuedConnection,
                                  Q_ARG(CommandEnum, kEnableTrigger), Q_ARG(int,0));
        QMetaObject::invokeMethod(serialPortInterface, "sendCommand", Qt::QueuedConnection,
                                  Q_ARG(CommandEnum, kEnableTriggerLogging), Q_ARG(int,1));
        QMetaObject::invokeMethod(serialPortInterface, "sendCommand", Qt::QueuedConnection,
                                  Q_ARG(CommandEnum, kEnableTrigger), Q_ARG(int,1));
        qDebug() << "triggers started true";
    }
    else {
        QMetaObject::invokeMethod(serialPortInterface, "sendCommand", Qt::QueuedConnection,
                                  Q_ARG(CommandEnum, kEnableTrigger), Q_ARG(int,0));
        QMetaObject::invokeMethod(serialPortInterface, "sendCommand", Qt::QueuedConnection,
                                  Q_ARG(CommandEnum, kEnableTriggerLogging), Q_ARG(int,0));
        QMetaObject::invokeMethod(serialPortInterface, "sendCommand", Qt::QueuedConnection,
                                  Q_ARG(CommandEnum, kEnableTrigger), Q_ARG(int,1));
        qDebug() << "triggers started false";
    }
}


void MazeControllerSerialPort::sendCommand(CommandEnum command)
{
  port->write((QString::number(command) + ";").toLocal8Bit());
  port->flush();
}

void MazeControllerSerialPort::sendCommand(CommandEnum command, int argument)
{
    port->write((QString::number(command) + "," + QString::number(argument) + ";").toLocal8Bit());
    port->flush();
}


MazeControllerSerialPort::MazeControllerSerialPort(QWaitCondition *_condition, QObject *parent) :
    QObject(parent)
{
    commandBuffer = "";
    loggingEnabled = false;
    port = NULL;
    waitingForResponse = false;
    condition = _condition;
}

MazeControllerSerialPort::~MazeControllerSerialPort()
{
    if (port != NULL)
        if (port->isOpen())
            port->close();
}

int MazeControllerSerialPort::initialize(QString name)
{
    QSerialPortInfo info(name);
    port = new QSerialPort(info);
    port->open(QIODevice::ReadWrite);
    if (!port->isOpen()){
        qDebug() << "Port not open";
        return 1;
    }
    port->setBaudRate(QSerialPort::Baud115200);
    QThread::msleep(1000);
    sendCommand(kEnableTrigger,0);
    sendCommand(kEnableTriggerLogging,0);
    sendCommand(kClearWellLog);
    port->clear();
    port->flush();
    connect(port, SIGNAL(readyRead()), this, SLOT(processSerialData()));
    return 0;
}

void MazeControllerSerialPort::processSerialData()
{
    int idx;
    QList<QByteArray> chunkedCommands;
    QList<QByteArray> parsedCommand;
    QByteArray command;
    static int counter = 0;
    ulong timestamp;
    char eventType;
    QList<int> pins;
    QList<int> wellLog;

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
                    CommandEnum receivedCommand = (CommandEnum) parsedCommand.at(0).toInt();
                    switch(receivedCommand) {
                    case (kStatus): // status message
                        emit statusMessageReceived(parsedCommand.at(1));
                        qDebug() << "maze controller status: " <<  parsedCommand.at(1);
                        break;
                    case (kEvent): // event
                        timestamp = parsedCommand.at(1).toULong();
                        eventType = parsedCommand.at(2).at(0);
                        if (eventType == 'T') { // trigger
                            emit frameTimestampEvent(timestamp,parsedCommand.at(3).toLong());
                        }
                        else if (eventType == 'W') {
                            emit wellVisitEvent(timestamp,parsedCommand.at(3).toInt());
                        }
                        else if (eventType == 'R') {
                            emit wellRewardEvent(timestamp,parsedCommand.at(3).toInt());
                        }
                        break;
                    case (kVersion):
                        emit versionString(parsedCommand.at(1));
                        qDebug() << "maze controller version:" << parsedCommand.at(1);
                        break;
                    case (kQueryPins):
                        pins.clear();
                        for (int j = 1; j < parsedCommand.length(); j++) {
                            pins.append(parsedCommand.at(j).toInt());
                        }
                        emit numberOfWells(pins.at(0));
                        pins.removeFirst(); // this gets rid of the well count
                        emit currentPins(pins);
                        break;
                    case (kQueryWellLog):
                        wellLog.clear();
                        for (int j = 1; j < parsedCommand.length()-1; j++) { //ignore total (last field)
                            wellLog.append(parsedCommand.at(j).toInt());
                        }
                        emit newRewardCounts(wellLog);
                        break;

                    default:
                        qDebug() << "Unexpected command received." << chunkedCommands.at(i);
                    }
                    if ((waitingForResponse) && (receivedCommand == expectedResponse)) {
                        qDebug() << "Waking up initializer";
                        condition->wakeAll();
                    }
                }
            }
        }
    }
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
