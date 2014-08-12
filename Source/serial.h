#include <QThread>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include "threads.h"

class Serial : public QObject
{
    Q_OBJECT
public:
    QString portname;
    QSerialPort port;
    ~Serial();
    void connect(QString portname);

public slots:
    void startTrigger(bool syncState);
    void startTriggerNoSync(void);
    void startTriggerSync(void);
    void stopTrigger(void);

signals:
    void triggersStopped(void);
    void triggersStarted(bool syncState);

};
