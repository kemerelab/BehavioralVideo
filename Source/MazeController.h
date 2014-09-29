#ifndef MAZE_CONTROLLER_H
#define MAZE_CONTROLLER_H

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QFile>
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

public slots:
    int connectToPort(QString portname);

    void startTrigger(bool syncState);
    void stopTrigger(void);

    void processSerialData(void);
    void initializeLogFile(QString filename);
    void beginWriting(void);
    void endWriting(void);

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

#endif
