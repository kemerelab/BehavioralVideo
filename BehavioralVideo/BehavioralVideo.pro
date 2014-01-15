#-------------------------------------------------
#
# Project created by QtCreator 2014-01-13T15:17:20
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BehavioralVideo
TEMPLATE = app

macx: INCLUDEPATH += /opt/local/include
macx: LIBS += -L"/opt/local/lib" -lavcodec -lavformat -lswscale -lavutil

SOURCES += main.cpp\
        mainwindow.cpp \
    camerainterface.cpp \
    videowidget.cpp \
    videoglwidget.cpp \
    videowriter.cpp

HEADERS  += mainwindow.h \
    camerainterface.h \
    videowidget.h \
    ffmpeg.h \
    videoglwidget.h \
    videowriter.h

FORMS    += mainwindow.ui

