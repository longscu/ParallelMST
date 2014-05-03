#-------------------------------------------------
#
# Project created by QtCreator 2014-05-03T12:10:31
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = ParallelMST
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

LIBS += -lstxxl -lgdal_i

SOURCES += main.cpp \
    tile.cpp \
    graph.cpp

HEADERS += \
    tile.h \
    graph.h

PRECOMPILED_HEADER = stable.h
