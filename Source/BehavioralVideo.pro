#-------------------------------------------------
#
# Project created by QtCreator 2014-01-13T15:17:20
#
#-------------------------------------------------

QT       += core gui multimedia
QT += serialport
QT += uitools
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets



TARGET = BehavioralVideo
TEMPLATE = app

unix: INCLUDEPATH += /usr/include /usr/include/flycapture
unix: LIBS += -L"/usr/lib" -lavcodec -lavformat -lswscale -lavutil -lflycapture
unix: QMAKE_CXXFLAGS += -D__STDC_CONSTANT_MACROS

QMAKE_CFLAGS += -g -O3

SOURCES += main.cpp\
        mainwindow.cpp \
    videowidget.cpp \
    videoglwidget.cpp \
    videowriter.cpp \
    fakecamerainterface.cpp \
    ptgreyinterface.cpp \
    dummycameracontroller.cpp \
    GenericCamera.cpp \
    GenericCameraController.cpp \
    datacontroller.cpp \
    MazeController.cpp \
    MazeInfoWindow.cpp

HEADERS  += mainwindow.h \
    videowidget.h \
    ffmpeg.h \
    videoglwidget.h \
    videowriter.h \
    fakecamerainterface.h \
    ptgreyinterface.h \
    threads.h \
    dummycameracontroller.h \
    GenericCamera.h \
    GenericCameraController.h \
    datacontroller.h \
    MazeController.h \
    MazeInfoWindow.h

FORMS    += mainwindow.ui

RESOURCES += \
    icons.qrc

