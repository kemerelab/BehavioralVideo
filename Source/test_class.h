#ifndef TEST_CLASS_H
#define TEST_CLASS_H

#include <QObject>
#include "GenericCamera.h"
#include "GenericCameraController.h"
#include <QList>

class TEST_CLASS : public QObject
{
    Q_OBJECT
public:
    explicit TEST_CLASS(QObject *parent = 0);

signals:

public slots:

};

#endif // TEST_CLASS_H
