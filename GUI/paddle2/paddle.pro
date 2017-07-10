#-------------------------------------------------
#
# Project created by QtCreator 2016-03-04T02:47:48
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = paddle
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    paddle.cpp \
    server.cpp

HEADERS  += mainwindow.h \
    CImg.h \
    defines.h \
    paddle.h \
    server.h \
    defines.h

FORMS    += mainwindow.ui

LIBS += -lX11 -ljpeg

CONFIG += c++11
