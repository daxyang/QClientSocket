#-------------------------------------------------
#
# Project created by QtCreator 2016-06-29T13:42:49
#
#-------------------------------------------------

QT       -= gui

TARGET = QClientSocket
TEMPLATE = lib

DEFINES += QCLIENTSOCKET_LIBRARY

SOURCES += qclientsocket.cpp

HEADERS += qclientsocket.h\
        qclientsocket_global.h \
    net_protocol.h


unix:macx {
INCLUDEPATH += ../QSlidingWindow
INCLUDEPATH += ../QSlidingWindowConsume

LIBS        += -L/usr/local/lib -lQSlidingWindow -lQSlidingWindowConsume
}
unix:macx {
    target.path = /usr/local/lib
    INSTALLS += target
}
