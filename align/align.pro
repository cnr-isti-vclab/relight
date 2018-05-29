QT += core widgets

QMAKE_CXXFLAGS += -std=c++14

TARGET = align
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    aligndialog.cpp \
    aligninspector.cpp

FORMS += \
    aligndialog.ui \
    aligninspector.ui

HEADERS += \
    aligndialog.h \
    aligninspector.h

