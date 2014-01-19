#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGridLayout>
#include "videowidget.h"
#include "videoglwidget.h"
#include "videowriter.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    VideoGLWidget *videoWidget;
    //VideoWidget *videoWidget;
    VideoWriter *videoWriter;

public slots:
    void openVideoFile();
    void enableVideoSaving();
    void disableVideoSaving();

signals:
    void initializeVideo(QString filename);

private:
    Ui::MainWindow *ui;
    QGridLayout *layout;
};

#endif // MAINWINDOW_H
