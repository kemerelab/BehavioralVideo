#ifndef VIDEOWRITER_H
#define VIDEOWRITER_H

#include <QObject>

class VideoWriter : public QObject
{
    Q_OBJECT
public:
    explicit VideoWriter(QObject *parent = 0);

signals:

public slots:

};

#endif // VIDEOWRITER_H
