QT += core gui widgets

QMAKE_CXXFLAGS += -std=c++14

TARGET = highlight
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    ballpickerdialog.cpp

HEADERS += \
    ballpickerdialog.h

FORMS += \
    ballpickerdialog.ui

