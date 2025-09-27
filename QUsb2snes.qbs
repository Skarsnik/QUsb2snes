import qbs

Project {
    minimumQbsVersion: "1.7.1"

    QtApplication {
        name : "QUsb2Snes"
        cpp.cxxLanguageVersion: "c++17"
        cpp.includePaths: ["devices/EmuNWAccess-qt", "./", "core/"]
        consoleApplication: false
        files: [
            "TODO",
            "client/usb2snesclient.cpp",
            "client/usb2snesclient.h",
            "core/types.h",
            "core/adevice.cpp",
            "core/adevice.h",
            "core/usb2snes.h",
            "core/wsserver.cpp",
            "core/wsserver.h",
            "core/devicefactory.cpp",
            "core/devicefactory.h",
            "core/wsservercommands.cpp",
            "core/localstorage.cpp",
            "core/localstorage.h",
            "core/aclient.h",
            "core/aclientprovider.h",
            "core/websocketclient.cpp",
            "core/websocketclient.h",
            "core/websocketprovider.cpp",
            "core/websocketprovider.h",
            "sqpath.h",
            "backward.hpp",

            "settings.hpp",
            "utils/ipsparse.cpp",
            "utils/ipsparse.h",
            "main.cpp",
            "qskarsnikringlist.hpp",
            "qusb2snes.rc",
            "ressources.qrc",

            "utils/rommapping/mapping_hirom.c",
            "utils/rommapping/mapping_lorom.c",
            "utils/rommapping/rommapping.c",
            "utils/rommapping/rominfo.c",

            "devices/sd2snesdevice.cpp",
            "devices/sd2snesdevice.h",
            "devices/luabridge.cpp",
            "devices/luabridge.h",
            "devices/luabridgedevice.cpp",
            "devices/luabridgedevice.h",
            "devices/snesclassic.cpp",
            "devices/snesclassic.h",
            "devices/deviceerror.cpp",
            "devices/deviceerror.h",
            "devices/emunetworkaccessdevice.cpp",
            "devices/emunetworkaccessdevice.h",
            "devices/emunetworkaccessfactory.cpp",
            "devices/emunetworkaccessfactory.h",
            "devices/retroarchfactory.cpp",
            "devices/retroarchfactory.h",
            "devices/retroarchhost.cpp",
            "devices/retroarchhost.h",
            "devices/retroarchdevice.cpp",
            "devices/retroarchdevice.h",
            "devices/sd2snesfactory.cpp",
            "devices/sd2snesfactory.h",
            "devices/snesclassicfactory.cpp",
            "devices/snesclassicfactory.h",
            "devices/EmuNWAccess-qt/emunwaccessclient.cpp",
            "devices/EmuNWAccess-qt/emunwaccessclient.h",
            "devices/remoteusb2sneswdevice.cpp",
            "devices/remoteusb2sneswdevice.h",
            "devices/remoteusb2sneswfactory.cpp",
            "devices/remoteusb2sneswfactory.h",


            "ui/appui.cpp",
            "ui/appuimenu.cpp",
            "ui/appui.h",
            "ui/appuipoptracker.cpp",
            "ui/appuiupdate.cpp",
            "ui/systraywidget.cpp",
            "ui/systraywidget.h",
            "ui/systraywidget.ui",
            "ui/diagnosticdialog.cpp",
            "ui/diagnosticdialog.h",
            "ui/diagnosticdialog.ui",

            "ui/wizard/deviceselectorpage.cpp",
            "ui/wizard/deviceselectorpage.h",
            "ui/wizard/deviceselectorpage.ui",
            "ui/wizard/devicesetupwizard.cpp",
            "ui/wizard/devicesetupwizard.h",
            "ui/wizard/devicesetupwizard.ui",
            "ui/wizard/lastpage.cpp",
            "ui/wizard/lastpage.h",
            "ui/wizard/lastpage.ui",
            "ui/wizard/nwapage.cpp",
            "ui/wizard/nwapage.h",
            "ui/wizard/nwapage.ui",
            "ui/wizard/retroarchpage.cpp",
            "ui/wizard/retroarchpage.h",
            "ui/wizard/retroarchpage.ui",
            "ui/wizard/sd2snespage.cpp",
            "ui/wizard/sd2snespage.h",
            "ui/wizard/sd2snespage.ui",
            "ui/wizard/snesclassicpage.cpp",
            "ui/wizard/snesclassicpage.h",
            "ui/wizard/snesclassicpage.ui",
        ]

        Group {
            name: "OSX"
            condition: qbs.targetPlatform.contains("macos")
            files: [
                "osx/appnap.h",
                "osx/appnap.mm"
            ]
        }

        Group {     // Properties for the produced executable
            fileTagsFilter: "application"
            qbs.install: true
        }
        Depends {
            name : "Qt";
            submodules : ["gui", "core", "widgets", "network", "serialport", "websockets"]
        }
        Properties {
            condition: qbs.targetPlatform.contains("macos")
            cpp.dynamicsLibraries: ["/System/Library/Frameworks/Foundation.framework/Versions/Current/Resources/BridgeSupport/Foundation.dylib"]
            cpp.frameworks: ["Foundation"]
        }
    }
}
