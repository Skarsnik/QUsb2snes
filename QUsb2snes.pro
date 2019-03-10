QT       += core gui websockets serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QUsb2Snes
TEMPLATE = app


SOURCES = adevice.cpp \
          appui.cpp \
          devicefactory.cpp \
          devices/sd2snesfactory.cpp \
          devices/snesclassicfactory.cpp \
          ipsparse.cpp \
          devices/luabridge.cpp \
          devices/luabridgedevice.cpp \
          devices/retroarchdevice.cpp \
          devices/retroarchfactory.cpp \
          main.cpp \
          rommapping/mapping_hirom.c \
          rommapping/mapping_lorom.c \
          rommapping/rommapping.c \
          devices/sd2snesdevice.cpp \
          devices/snesclassic.cpp \
          wsserver.cpp \
          wsservercommands.cpp

HEADERS = adevice.h \
          appui.h \
          devicefactory.h \
          devices/sd2snesfactory.h \
          devices/snesclassicfactory.h \
          devices/retroarchfactory.h \
          ipsparse.h \
          devices/luabridge.h \
          devices/luabridgedevice.h \
          devices/retroarchdevice.h \
          rommapping/rommapping.h \
          devices/sd2snesdevice.h \
          devices/snesclassic.h \
          usb2snes.h \
          wsserver.h

macx: {
	SOURCES += osx/appnap.mm
	HEADERS += osx/appnap.h
	LIBS += -framework Foundation
}

RESOURCES = ressources.qrc

ICONS = cheer128x128.icns

RC_FILE = qusb2snes.rc
