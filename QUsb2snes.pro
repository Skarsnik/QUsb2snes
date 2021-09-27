QT       += core websockets serialport network

TARGET = QUsb2Snes
TEMPLATE = app
GIT_TAG_VERSION=$$system(git describe --always --tags)
DEFINES += GIT_TAG_VERSION=\\\"$$GIT_TAG_VERSION\\\"


UISOURCES = ui/appui.cpp \
            ui/tempdeviceselector.cpp
UIHEADERS = ui/appui.h \
            ui/tempdeviceselector.h

equals(QUSB2SNES_NOGUI, 1) {
    message("building QUsb2Snes in NOGUI mode")
    DEFINES += "QUSB2SNES_NOGUI=1"
    QT -= gui
} else {
    QT += gui widgets
    FORMS =  ui/tempdeviceselector.ui
    SOURCES = $$UISOURCES
    HEADERS = $$UIHEADERS
}

include($$PWD/devices/EmuNWAccess-qt/EmuNWAccess-qt.pri)

INCLUDEPATH += core/ $$PWD

SOURCES += main.cpp \
          core/adevice.cpp \
          core/devicefactory.cpp \
          core/wsserver.cpp \
          core/wsservercommands.cpp \
          devices/sd2snesfactory.cpp \
          devices/snesclassicfactory.cpp \
          devices/deviceerror.cpp \
          devices/luabridge.cpp \
          devices/luabridgedevice.cpp \
          devices/retroarchdevice.cpp \
          devices/retroarchfactory.cpp \
          devices/retroarchhost.cpp \
          devices/emunetworkaccessfactory.cpp \
          devices/emunetworkaccessdevice.cpp \
          devices/sd2snesdevice.cpp \
          devices/snesclassic.cpp \
          utils/ipsparse.cpp \
          utils/rommapping/mapping_hirom.c \
          utils/rommapping/mapping_lorom.c \
          utils/rommapping/rommapping.c \
          utils/rommapping/rominfo.c

HEADERS += core/adevice.h \
          core/devicefactory.h \
          core/usb2snes.h \
          core/wsserver.h \
          devices/deviceerror.h \
          devices/sd2snesfactory.h \
          devices/snesclassicfactory.h \
          devices/retroarchfactory.h \
          devices/luabridge.h \
          devices/luabridgedevice.h \
          devices/retroarchdevice.h \
          devices/retroarchhost.h \
          devices/emunetworkaccessfactory.h \
          devices/emunetworkaccessdevice.h \
          devices/snesclassic.h
          devices/sd2snesdevice.h
          utils/ipsparse.h \
          utils/rommapping/rommapping.h \
          utils/rommapping/rominfo.h

macx: {
        message("MAC OS BUILD")
	SOURCES += osx/appnap.mm
	HEADERS += osx/appnap.h
	LIBS += -framework Foundation
	QMAKE_INFO_PLIST = osx/Info.plist
	ICON = cheer128x128.icns
}

RESOURCES = ressources.qrc


RC_FILE = qusb2snes.rc


TRANSLATIONS = Translations\qusb2snes_fr.ts
