#ifndef MAZE_CONTROLLER_H
#define MAZE_CONTROLLER_H

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QFile>
#include <QWidget>
#include "GenericCameraController.h"

bool isMazeControllerConnected();

class MazeController : public GenericCameraController
{

    Q_OBJECT
public:
    MazeController(QObject *parent = 0);
    QString portname;
    QSerialPort *port;
    ~MazeController();
    QString versionString;

    int numberOfWells;
    QStringList availablePins;
    QList<int> beamBreakPins;
    QList<int> pumpPins;
    int cameraPin;

public slots:
    int connectToPort(QString portname);

    void startTrigger(bool syncState);
    void stopTrigger(void);

    void processSerialData(void);
    void initializeLogFile(QString filename);
    void beginWriting(void);
    void endWriting(void);

    void changeCameraFrameRate(int);
    void changeCameraTriggerPin(int);
    void testWell(int);
    void resetWellCounts(void);

signals:
    void serialDataReceived(QByteArray);
    void statusMessageReceived(QString);
    void frameTimestampEvent(ulong time, ulong count);
    void wellVisitEvent(ulong time,char well);
    void wellRewardEvent(ulong time,char well, int count, int total);

private:
    QFile *logFile;
    bool loggingEnabled;
    QByteArray commandBuffer;

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
