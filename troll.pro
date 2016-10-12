#-------------------------------------------------
#
# Project created by QtCreator 2016-10-12T11:19:32
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = troll
TEMPLATE = app


SOURCES +=\
        mainwindow.cxx \
	main.cxx \
    libtroll/libtroll.cxx

HEADERS  += mainwindow.hxx \
    libtroll/dwarf.h \
    libtroll/libtroll.hxx

FORMS    += mainwindow.ui

INCLUDEPATH += libtroll/
