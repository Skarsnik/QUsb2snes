QT       += core websockets serialport network

TARGET = QUsb2Snes
TEMPLATE = app
GIT_TAG_VERSION=$$system(git describe --always --tags)
DEFINES += GIT_TAG_VERSION=\\\"$$GIT_TAG_VERSION\\\"


UISOURCES = ui/appui.cpp \
            ui/appuimenu.cpp \
            ui/apppoptracker.cpp \
            ui/tempdeviceselector.cpp \
            ui/diagnosticdialog.cpp
UIHEADERS = ui/appui.h \
            ui/tempdeviceselector.h \
            ui/diagnosticdialog.h

equals(QUSB2SNES_NOGUI, 1) {
    message("building QUsb2Snes in NOGUI mode")
    DEFINES += "QUSB2SNES_NOGUI=1"
    QT -= gui
} else {
    QT += gui widgets
    FORMS =  ui/tempdeviceselector.ui \
             ui/diagnosticdialog.ui
    SOURCES = $$UISOURCES
    HEADERS = $$UIHEADERS
}

include($$PWD/devices/EmuNWAccess-qt/EmuNWAccess-qt.pri)

SOURCES += adevice.cpp \
          devicefactory.cpp \
          devices/sd2snesfactory.cpp \
          devices/snesclassicfactory.cpp \
          ipsparse.cpp \
          devices/deviceerror.cpp \
          devices/luabridge.cpp \
          devices/luabridgedevice.cpp \
          devices/retroarchdevice.cpp \
          devices/retroarchfactory.cpp \
          devices/retroarchhost.cpp \
          devices/emunetworkaccessfactory.cpp \
          devices/emunetworkaccessdevice.cpp \
          main.cpp \
          rommapping/mapping_hirom.c \
          rommapping/mapping_lorom.c \
          rommapping/rommapping.c \
          rommapping/rominfo.c \
          devices/sd2snesdevice.cpp \
          devices/snesclassic.cpp \
          localstorage.cpp \
          wsserver.cpp \
          wsservercommands.cpp

HEADERS += adevice.h \
          devicefactory.h \
          devices/deviceerror.h \
          devices/sd2snesfactory.h \
          devices/snesclassicfactory.h \
          devices/retroarchfactory.h \
          ipsparse.h \
          devices/luabridge.h \
          devices/luabridgedevice.h \
          devices/retroarchdevice.h \
          devices/retroarchhost.h \
          devices/emunetworkaccessfactory.h \
          devices/emunetworkaccessdevice.h \
          rommapping/rommapping.h \
          rommapping/rominfo.h \
          devices/sd2snesdevice.h \
          devices/snesclassic.h \
          localstorage.h \
          usb2snes.h \
          wsserver.h

macx: {
        message("MAC OS BUILD")
	SOURCES += osx/appnap.mm
	HEADERS += osx/appnap.h
	LIBS += -framework Foundation
	QMAKE_INFO_PLIST = osx/Info.plist
        ICON = ui/icons/cheer128x128.icns
}

RESOURCES = ressources.qrc


RC_FILE = qusb2snes.rc


TRANSLATIONS = Translations\qusb2snes_fr.ts
