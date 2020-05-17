QT += core gui widgets concurrent

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
    graphics_view_zoom.cpp \
    ballwidget.cpp \
    progress.cpp

HEADERS += \
    ballpickerdialog.h \
    mainwindow.h \
    ball.h \
    imagedialog.h \
    graphics_view_zoom.h \
    ballwidget.h \
    progress.h

FORMS += \
    ballpickerdialog.ui \
    mainwindow.ui \
    imagedialog.ui \
    ballwidget.ui \
    progress.ui

RESOURCES += \
    icons.qrc

