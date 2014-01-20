#ifndef CAMERAINTERFACE_H
#define CAMERAINTERFACE_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include "ffmpeg.h"

class FakeVideoGenerator : public QObject
{
    Q_OBJECT
public:
    explicit FakeVideoGenerator(QObject *parent = 0);
    ~FakeVideoGenerator(void);

signals:
    void newFrame(QImage frame);

public slots:
    void GenerateNextFrame(void);
    void StartVideo(void);

public:

private:
    QTimer *frameTimer;
    int width, height;
    int frameIdx;
    QImage *currentFrame;
    AVFrame *currentFrame_YUV;
    void *currentFrame_YUV_buf;
    AVFrame *currentFrame_RGB;
    struct SwsContext *sws_ctx;
};

#endif // CAMERAINTERFACE_H
