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
};
