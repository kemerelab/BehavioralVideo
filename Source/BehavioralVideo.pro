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
    VideoGLWidget.cpp \
    VideoWriter.cpp \
    CameraInterfaces/FakeCamera.cpp \
    CameraInterfaces/PtGrey.cpp \
    dummycameracontroller.cpp \
    CameraInterfaces/GenericCamera.cpp \
    GenericCameraController.cpp \
    DataController.cpp \
    MazeController.cpp \
    MazeInfoWindow.cpp

HEADERS  += mainwindow.h \
    FFMPEG.h \
    VideoGLWidget.h \
    VideoWriter.h \
    CameraInterfaces/FakeCamera.h \
    CameraInterfaces/PtGrey.h \
    Threads.h \
    dummycameracontroller.h \
    GenericCamera.h \
    GenericCameraController.h \
    DataController.h \
    MazeController.h \
    MazeInfoWindow.h

FORMS    += mainwindow.ui

RESOURCES += \
    icons.qrc

