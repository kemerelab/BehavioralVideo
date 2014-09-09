#ifndef GENERIC_CAMERA_H
#define GENERIC_CAMERA_H

#include <QObject>
#include "ffmpeg.h"
#include <QImage>
#include "videowriter.h"

class GenericCameraInterface : public QObject
{
    Q_OBJECT
public:
    explicit GenericCameraInterface(QObject *parent = 0);
     ~GenericCameraInterface(void);

signals:
    void newFrame(QImage frame);
    void capturingStarted(void);
    void capturingEnded(void);

public slots:
    virtual void Initialize(void) = 0;
    virtual void StartCapture(bool enableStrobe) = 0;
    virtual void StopCapture(void) = 0;
    void StartCameraCaptureSync(void) { StartCapture(true); };
    void StartCameraCaptureAsync(void) { StartCapture(false); };
    void StopAndRestartCaptureSync(void) { StopCapture(); StartCapture(true); };
    void StopAndRestartCaptureAsync(void)  { StopCapture(); StartCapture(false); };

public:
    int width, height;
    QImage *currentFrame;

    QString cameraName;
    bool isCapturing;
    bool isInitialized;

};

Q_DECLARE_METATYPE(GenericCameraInterface*)

#endif // GenericCameraInterface_H
