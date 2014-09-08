#ifndef DUMMYCAMERACONTROLLER_H
#define DUMMYCAMERACONTROLLER_H

#include <QObject>

class DummyCameraController : public QObject
{
    Q_OBJECT
public:
    explicit DummyCameraController(QObject *parent = 0);
    ~DummyCameraController();

signals:
    void triggersStopped(void);
    void triggersStarted(bool syncState);

public slots:
    void startTrigger(bool syncState);
    void startTriggerNoSync(void);
    void startTriggerSync(void);
    void stopTrigger(void);
};

#endif // DUMMYCAMERACONTROLLER_H
