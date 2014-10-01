#ifndef MAZEINFOWINDOW_H
#define MAZEINFOWINDOW_H

#include <QWidget>
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QList>
#include <QPushButton>
#include <QGridLayout>

class MazeInfoWindow : public QDockWidget
{
    Q_OBJECT
public:
    explicit MazeInfoWindow(int nWells, QWidget *parent = 0);

signals:
    void rewardWell(int);

public slots:
    void setupWellButtons(int nWells);
    void newLogText(QByteArray text);
    void newStatusMessage(QString text);
    void newFrameTimestampEvent(ulong time, ulong count);
    void newWellVisitEvent(ulong time,int well);
    void newWellRewardEvent(ulong time, int well);
    void newWellCounts(QList<int> newCounts);

private:
    QWidget *container;
    QGridLayout *layout;
    QPlainTextEdit *logWindow;
    QByteArray tmpCommand;
    int numberOfWells;
    QList<int> wellCounts;
    QList<QPushButton *> wellButtons;
};

#endif // MAZEINFOWINDOW_H
