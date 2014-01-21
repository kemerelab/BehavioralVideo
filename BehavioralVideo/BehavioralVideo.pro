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
    ptgreyinterface.cpp

HEADERS  += mainwindow.h \
    videowidget.h \
    ffmpeg.h \
    videoglwidget.h \
    videowriter.h \
    fakecamerainterface.h \
    ptgreyinterface.h \
    threads.h

FORMS    += mainwindow.ui

