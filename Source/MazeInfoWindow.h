#ifndef MAZEINFOWINDOW_H
#define MAZEINFOWINDOW_H

#include <QWidget>
#include <QDockWidget>
#include <QPlainTextEdit>

class MazeInfoWindow : public QDockWidget
{
    Q_OBJECT
public:
    explicit MazeInfoWindow(QWidget *parent = 0);

signals:

public slots:
    void newLogText(QByteArray text);
    void newStatusMessage(QString text);
    void newFrameTimestampEvent(ulong time, ulong count);
    void newWellVisitEvent(ulong time,char well);
    void newWellRewardEvent(ulong time,char well, int count, int total);


private:
    QPlainTextEdit *logWindow;
    QByteArray tmpCommand;
};

#endif // MAZEINFOWINDOW_H
