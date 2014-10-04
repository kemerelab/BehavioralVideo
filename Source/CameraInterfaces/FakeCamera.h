#ifndef FAKECAMERAINTERFACE_H
#define FAKECAMERAINTERFACE_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include "FFMPEG.h"
#include "GenericCamera.h"

class FakeVideoGenerator : public GenericCameraInterface
{
    Q_OBJECT
public:
    explicit FakeVideoGenerator(QObject *parent = 0);
    ~FakeVideoGenerator(void);


public slots:
    void GenerateNextFrame(void);
    void Initialize(void);
    void StartCapture(bool enableStrobe);
    void StopCapture(void);

public:

private:
    QTimer *frameTimer;
    int frameIdx;
    QImage *currentFrame;
    AVFrame *currentFrame_YUV;
    void *currentFrame_YUV_buf;
    AVFrame *currentFrame_RGB;
    struct SwsContext *sws_ctx;
};

#endif // FAKECAMERAINTERFACE_H
