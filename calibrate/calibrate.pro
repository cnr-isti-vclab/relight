QT += core
#QT -= gui

TARGET = calibrate
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++14

TEMPLATE = app

SOURCES += main.cpp

INCLUDEPATH += /usr/local/include/libraw /home/ponchio/build/rawspeed-3.1/src/librawspeed/
LIBS += -L/usr/local/lib -lraw /home/ponchio/build/rawspeed-3.1/build/src/librawspeed.a

LIBS += ../levmar-2.6/liblevmar.a -lm -llapack -lblas
