#ifndef POINTGREY_H
#define POINTGREY_H

#include <QObject>
#include "FlyCapture2.h"

using namespace FlyCapture2;

class PointGrey : public QObject
{
    Q_OBJECT
public:
    explicit PointGrey(QObject *parent = 0);

signals:

public slots:

};

void PrintBuildInfo();
void PrintCameraInfo( CameraInfo* pCamInfo );
void PrintError( Error error );

#endif // POINTGREY_H
