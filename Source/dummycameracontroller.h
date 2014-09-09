#ifndef DUMMYCAMERACONTROLLER_H
#define DUMMYCAMERACONTROLLER_H

#include <QObject>
#include "GenericCameraController.h"

class DummyCameraController : public GenericCameraController
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
    void stopTrigger(void);

    //void savingStateMachine(void);


};

#endif // DUMMYCAMERACONTROLLER_H
