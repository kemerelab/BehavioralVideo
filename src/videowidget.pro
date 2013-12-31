TEMPLATE = app

 QT += core gui widgets multimedia

 HEADERS = \
     videoplayer.h \
     videowidget.h \
     videowidgetsurface.h \
    pointgrey.h

 SOURCES = \
     main.cpp \
     videoplayer.cpp \
     videowidget.cpp \
     videowidgetsurface.cpp \
    pointgrey.cpp

 # install
 target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/videowidget
 sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png images
 sources.path = $$[QT_INSTALL_EXAMPLES]/multimedia/videowidget
 INSTALLS += target sources

