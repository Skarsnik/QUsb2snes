#-------------------------------------------------
#
# Project created by QtCreator 2018-12-08T16:40:44
#
#-------------------------------------------------

QT       += core gui widgets websockets

TARGET = QFile2Snes
TEMPLATE = app

INCLUDEPATH += ../client

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17

SOURCES += \
        main.cpp \
        qfile2snesw.cpp \
        usb2snesfilemodel.cpp \
        ../client/usb2snesclient.cpp \
        myfilesystemmodel.cpp

HEADERS += \
        qfile2snesw.h \
        usb2snesfilemodel.h \
        ../client/usb2snesclient.h \
        myfilesystemmodel.h

FORMS += \
        qfile2snesw.ui \

RC_FILE = qfile2snes.rc


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    qfile2snes.rc \
    Readme.md

TRANSLATIONS = Translations/qfile2snes_fr.ts

