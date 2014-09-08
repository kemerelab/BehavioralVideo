#include <QThread>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include "threads.h"


bool isSerialControllerConnected();

class SerialCameraController : public QObject
{
    Q_OBJECT
public:
    QString portname;
    QSerialPort port;
    ~SerialCameraController();
    int connect(QString portname);

public slots:
    void startTrigger(bool syncState);
    void startTriggerNoSync(void);
    void startTriggerSync(void);
    void stopTrigger(void);

signals:
    void triggersStopped(void);
    void triggersStarted(bool syncState);

};
