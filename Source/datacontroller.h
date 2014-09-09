#ifndef DATA_CONTROLLER_H
#define DATA_CONTROLLER_H

#include "GenericCamera.h"
#include "GenericCameraController.h"
#include "videowriter.h"
#include <QList>

enum TriggerType {
    NO_SELECTION,
    NO_CAMERA_TRIGGER,
    EXTERNAL_CAMERA_TRIGGER
};

Q_DECLARE_METATYPE(TriggerType)

enum SavingState {
    NOT_SAVING,
    READY_TO_WRITE,
    CURRENTLY_WRITING
};


class DataController : public QObject
{
    Q_OBJECT
public:
    explicit DataController(QObject *parent = 0);

signals:
    void updateSavingMenus(SavingState);

public slots:
    void stopVideo(void);
    void stopVideoWriting(void);
    void initializeVideoWriting(QString filename);
    void startVideoStreaming(bool writeToDisk);
    void startVideoRecording(void) { startVideoStreaming(true); }
    void startVideoViewing(void) { startVideoStreaming(false); }
    void useTriggering(TriggerType trigger) { triggerType = trigger; }
    void registerCameraAndWriter(GenericCameraInterface *camera, VideoWriter *writer);
    void registerCameraController(GenericCameraController *controller);

private:
    QList<GenericCameraInterface *> cameraList;
    QList<VideoWriter *> videoWriterList;
    GenericCameraController *cameraController;
    int numCameras;
    TriggerType triggerType;
    bool videoIsStreaming;

};

Q_DECLARE_METATYPE(DataController*)


#endif // DATA_CONTROLLER_H
