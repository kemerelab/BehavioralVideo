#ifndef GENERIC_CONTROLLER_H
#define GENERIC_CONTROLLER_H


#include <QObject>

class GenericCameraController : public QObject
{
    Q_OBJECT

public:
    GenericCameraController(QObject *parent = 0);
     ~GenericCameraController(void);

public slots:
    virtual void startTrigger(bool syncState) = 0;
    virtual void startTriggerNoSync(void) { startTrigger(false); };
    virtual void startTriggerSync(void) { startTrigger(true); };
    virtual void stopTrigger(void) = 0;
    virtual void initializeLogFile(QString filename) { };
    virtual void beginWriting(void) { };
    virtual void endWriting(void) { };

signals:

public:
    int numCameras;
};

Q_DECLARE_METATYPE(GenericCameraController*)

#endif // GENERIC_CAMERA_CONTROLLER_H
