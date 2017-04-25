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
    troll.cxx \
    blackmagic.cxx \
    gdb-remote.cxx \
    external-sources/capstone/cs.c \
    external-sources/capstone/MCInst.c \
    external-sources/capstone/MCInstrDesc.c \
    external-sources/capstone/MCRegisterInfo.c \
    external-sources/capstone/SStream.c \
    external-sources/capstone/utils.c \
    external-sources/capstone/arch/ARM/ARMDisassembler.c \
    external-sources/capstone/arch/ARM/ARMInstPrinter.c \
    external-sources/capstone/arch/ARM/ARMMapping.c \
    external-sources/capstone/arch/ARM/ARMModule.c

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
    disassembly.hxx \
    dwarf-evaluator.hxx \
    troll.hxx \
    blackmagic.hxx \
    gdb-remote.hxx

FORMS    += mainwindow.ui \
    notification.ui

INCLUDEPATH += libtroll/ sforth/ external-sources/elfio/ external-sources/capstone/include

DEFINES += CORE_CELLS_COUNT="128*1024" STACK_DEPTH=32 TEST_DRIVE_MODE=0 CAPSTONE_USE_SYS_DYN_MEM CAPSTONE_HAS_ARM

DISTFILES += \
    unwinder.fs \
    dwarf-expression.fs

RESOURCES += \
    resources.qrc
