QT       += core gui widgets websockets

TARGET = SendToSd2Snes
TEMPLATE = app

INCLUDEPATH += ../client

CONFIG += c++11

SOURCES += \
    mainsendto.cpp \
    usb2snesfilemodel.cpp \
    ../client/usb2snes.cpp \
    sendtodialog.cpp \
    dirchangedialog.cpp


HEADERS += \
    usb2snesfilemodel.h \
    ../client/usb2snes.h \
    sendtodialog.h \
    dirchangedialog.h \

FORMS += \
    sendtodialog.ui \
    dirchangedialog.ui

RC_FILE = qfile2snes.rc

DISTFILES += \
    qfile2snes.rc

TRANSLATIONS = Translations\sendtodialog_fr.ts
