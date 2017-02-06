#-------------------------------------------------
#
# Project created by QtCreator 2017-02-01T22:21:53
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = visualPE
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    Src/HexTextEditor/hextexteditor.cpp \
    Src/HexTextEditor/tokenlist.cpp \
    Src/TypeTree.cpp \
    Src/pe/pe.cpp \
    Src/HexTextEditor/hexeditdialog.cpp \
    Src/HexTextEditor/hextexteditorcolorConfigure.cpp

HEADERS  += mainwindow.h \
    Src/HexTextEditor/hextexteditor.h \
    Src/HexTextEditor/tokenlist.h \
    Src/TypeTree.h \
    Src/pe/pe.h \
    Src/HexTextEditor/hexeditdialog.h \
    Src/HexTextEditor/hextexteditorcolorConfigure.h

FORMS    += mainwindow.ui
CONFIG+=thread