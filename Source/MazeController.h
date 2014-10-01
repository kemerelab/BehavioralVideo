#ifndef MAZE_CONTROLLER_H
#define MAZE_CONTROLLER_H

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QFile>
#include <QWidget>
#include <QWaitCondition>
#include <QDebug>
#include <QMutex>
#include "GenericCameraController.h"
#include "Arduino/Common/BehaviorInterfaceCommands.h"

QStringList listSerialPorts();


class MazeControllerSerialPort : public QObject
{
    Q_OBJECT
public:
    explicit MazeControllerSerialPort(QWaitCondition *condition, QObject *parent = 0);
    ~MazeControllerSerialPort(void);
private:
    QSerialPort *port;
    QByteArray commandBuffer;

    QWaitCondition *condition;
    bool waitingForResponse;
    CommandEnum expectedResponse;

    QFile *logFile;
    bool loggingEnabled;


public slots:
    int  initialize(QString name);
    void processSerialData(void);
    void sendCommand(CommandEnum command);
    void sendCommand(CommandEnum command, int argument);

    void expectResponse(CommandEnum command) {
            waitingForResponse = true;
            expectedResponse = command;
    };
    void enableLogging(void) { loggingEnabled = true; };
    void disableLogging(void) { loggingEnabled = false; };

signals:
    void versionString(QString);
    void numberOfWells(int);
    void currentPins(QList<int>);
    void statusMessageReceived(QString);
    void frameTimestampEvent(ulong time, ulong count);
    void wellVisitEvent(ulong, int);
    void wellRewardEvent(ulong, int);
    void newRewardCounts(QList<int>);
};


class MazeController : public GenericCameraController
{

    Q_OBJECT
public:
    MazeController(QObject *parent = 0);
    ~MazeController();
    MazeControllerSerialPort *serialPortInterface;

    QString versionString;

    int numberOfWells;
    QStringList availablePins;
    QList<int> beamBreakPins;
    QList<int> pumpPins;
    int cameraPin;
    QMutex mutex;
    QWaitCondition condition;

public slots:
    int connectToPort(QString portname);

    void startTrigger(bool syncState);
    void stopTrigger(void);

    void initializeLogFile(QString filename);
    void beginWriting(void);
    void endWriting(void);

    void changeCameraFrameRate(int);
    void changeCameraTriggerPin(int);
    void setNumberOfWells(int n) {numberOfWells = n;};
    void setVersionString(QString s) {versionString = s;};
    void setPins(QList<int> pinList);
    void testWell(int);
    void resetWellCounts(void);

signals:

private:
    QFile *logFile;

    QThread *portThread;

};

class MazeControllerSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MazeControllerSettingsWidget(MazeController *controller, QWidget *parent = 0);
     ~MazeControllerSettingsWidget(void);

public slots:
    void changeCameraFrameRate(QString);
    void changeCameraTriggerPin(int);

signals:
    void newCameraFrameRate(int);
    void newCameraTriggerPin(int);

private:
};


#endif
