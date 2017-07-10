#-------------------------------------------------
#
# Project created by QtCreator 2016-06-16T14:58:26
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ControlWindowPaddle
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

LIBS += -lX11 -ljpeg

CONFIG += c++11
