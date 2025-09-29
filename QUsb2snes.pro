QT       += core websockets serialport network

TARGET = QUsb2Snes
TEMPLATE = app
GIT_TAG_VERSION=$$system(git describe --always --tags)

exists("sq_project_forced_version.pri") {
        include("sq_project_forced_version.pri")
	GIT_TAG_VERSION=$$SQ_PROJECT_FORCED_VERSION
}

DEFINES += GIT_TAG_VERSION=\\\"$$GIT_TAG_VERSION\\\"


UISOURCES = ui/appui.cpp \
            ui/appuimenu.cpp \
            ui/appuipoptracker.cpp \
	    ui/appuiupdate.cpp \
            ui/diagnosticdialog.cpp \
	    ui/systraywidget.cpp \
            ui/wizard/deviceselectorpage.cpp \
            ui/wizard/devicesetupwizard.cpp \
            ui/wizard/lastpage.cpp \
            ui/wizard/retroarchpage.cpp \
            ui/wizard/sd2snespage.cpp \
            ui/wizard/nwapage.cpp \
            ui/wizard/snesclassicpage.cpp
UIHEADERS = ui/appui.h \
            ui/diagnosticdialog.h \
	    ui/systraywidget.h \
            ui/wizard/deviceselectorpage.h \
            ui/wizard/devicesetupwizard.h \
            ui/wizard/lastpage.h \
            ui/wizard/retroarchpage.h \
            ui/wizard/sd2snespage.h \
            ui/wizard/nwapage.h \
            ui/wizard/snesclassicpage.h


equals(QUSB2SNES_NOGUI, 1) {
    message("building QUsb2Snes in NOGUI mode")
    DEFINES += "QUSB2SNES_NOGUI=1"
    QT -= gui
} else {
    QT += gui widgets
    FORMS =  ui/diagnosticdialog.ui \
	     ui/systraywidget.ui \
             ui/wizard/deviceselectorpage.ui \
             ui/wizard/devicesetupwizard.ui \
             ui/wizard/lastpage.ui \
             ui/wizard/retroarchpage.ui \
             ui/wizard/sd2snespage.ui \
             ui/wizard/nwapage.ui \
             ui/wizard/snesclassicpage.ui

    SOURCES = $$UISOURCES
    HEADERS = $$UIHEADERS
}

include($$PWD/devices/EmuNWAccess-qt/EmuNWAccess-qt.pri)

INCLUDEPATH += core/

SOURCES += core/adevice.cpp \
          core/devicefactory.cpp \
          core/localstorage.cpp \
          core/wsserver.cpp \
          core/wsservercommands.cpp \
          core/websocketprovider.cpp \
          core/websocketclient.cpp \
          devices/sd2snesdevice.cpp \
          devices/snesclassic.cpp \
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
          devices/remoteusb2sneswfactory.cpp \
          devices/remoteusb2sneswdevice.cpp \
          main.cpp \
          utils/rommapping/mapping_hirom.c \
          utils/rommapping/mapping_lorom.c \
          utils/rommapping/rommapping.c \
          utils/rommapping/rominfo.c \
          utils/ipsparse.cpp \
          client/usb2snesclient.cpp

HEADERS +=core/adevice.h \
          core/localstorage.h \
          core/usb2snes.h \
          core/devicefactory.h \
          core/aclient.h \
          core/websocketclient.h \
          core/websocketprovider.h \
          core/wsserver.h \
          devices/deviceerror.h \
          devices/sd2snesfactory.h \
          devices/snesclassicfactory.h \
          devices/retroarchfactory.h \
          devices/remoteusb2sneswfactory.h \
          devices/remoteusb2sneswdevice.h \
          devices/luabridge.h \
          devices/luabridgedevice.h \
          devices/retroarchdevice.h \
          devices/retroarchhost.h \
          devices/emunetworkaccessfactory.h \
          devices/emunetworkaccessdevice.h \
          devices/sd2snesdevice.h \
          devices/snesclassic.h \
          utils/ipsparse.h \
          utils/rommapping/rommapping.h \
          utils/rommapping/rominfo.h \
          client/usb2snesclient.h \
	  sqpath.h \
          settings.hpp

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
