QT += core gui widgets

QMAKE_CXXFLAGS += -std=c++14

TARGET = highlight
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    ballpickerdialog.cpp \
    mainwindow.cpp \
    ball.cpp \
    imagedialog.cpp \
    graphics_view_zoom.cpp

HEADERS += \
    ballpickerdialog.h \
    mainwindow.h \
    ball.h \
    imagedialog.h \
    graphics_view_zoom.h

FORMS += \
    ballpickerdialog.ui \
    mainwindow.ui \
    imagedialog.ui

RESOURCES += \
    icons.qrc

