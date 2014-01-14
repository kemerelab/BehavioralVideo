#ifndef CAMERAINTERFACE_H
#define CAMERAINTERFACE_H

#include <QObject>
#include <QImage>
#include "ffmpeg.h"

class CameraInterface : public QObject
{
    Q_OBJECT
public:
    explicit CameraInterface(QObject *parent = 0);
    ~CameraInterface(void);

signals:
    void newFrame(QImage frame);

public slots:
    void GenerateNextFrame(void);

public:

private:
    int width, height;
    int frameIdx;
    QImage *currentFrame;
    AVFrame *currentFrame_YUV;
    AVFrame *currentFrame_RGB;
    struct SwsContext *sws_ctx;
};

#endif // CAMERAINTERFACE_H
