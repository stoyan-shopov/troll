#-------------------------------------------------
#
# Project created by QtCreator 2016-10-12T11:19:32
#
#-------------------------------------------------

QT       += core gui serialport xml
#CONFIG	+= console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = troll
TEMPLATE = app
QMAKE_CXXFLAGS += -Wno-sign-compare


SOURCES +=\
	main.cxx \
    libtroll/libtroll.cxx \
    sforth/engine.c \
    sforth/sf-opt-file.c \
    sforth/sf-opt-prog-tools.c \
    sforth/sf-opt-string.c \
    sforth.cxx \
    cortexm0.cxx \
    cortexm0-sfext.c \
    target-corefile.cxx \
    blackstrike.cxx \
    dwarf-evaluator.cxx \
    troll.cxx

HEADERS  += \
    libtroll/dwarf.h \
    libtroll/libtroll.hxx \
    sforth.hxx \
    cortexm0.hxx \
    util.hxx \
    target.hxx \
    target-corefile.hxx \
    blackstrike.hxx \
    registercache.hxx \
    memory.hxx \
    s-record.hxx \
    flash-memory-writer.hxx \
    disassembly.hxx \
    dwarf-evaluator.hxx \
    troll.hxx

FORMS    += mainwindow.ui \
    notification.ui

INCLUDEPATH += libtroll/ sforth/

DEFINES += ENGINE_32BIT CORE_CELLS_COUNT="128*1024" STACK_DEPTH=64 TEST_DRIVE_MODE=1

DISTFILES += \
    unwinder.fs \
    dwarf-expression.fs

RESOURCES += \
    resources.qrc
