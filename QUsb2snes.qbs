import qbs

Project {
    minimumQbsVersion: "1.7.1"

    QtApplication {
        name : "QUsb2Snes"
        cpp.cxxLanguageVersion: "c++11"
        cpp.includePaths: ["devices/EmuNWAccess-qt", "core", "."]
        consoleApplication: false
        files: [
            "core/aclient.h",
            "core/aclientprovider.h",
            "core/adevice.cpp",
            "core/adevice.h",
            "core/devicefactory.cpp",
            "core/devicefactory.h",
            "core/types.h",
            "core/usb2snes.h",
            "core/websocketclient.cpp",
            "core/websocketclient.h",
            "core/websocketprovider.cpp",
            "core/websocketprovider.h",
            "core/wsserver.cpp",
            "core/wsserver.h",
            "core/wsservercommands.cpp",
            "core/localstorage.cpp",
            "core/localstorage.h",
            "ui/appui.cpp",
            "ui/appui.h",
            "ui/appuimenu.cpp",
            "ui/appuipoptracker.cpp",
            "ui/tempdeviceselector.cpp",
            "ui/tempdeviceselector.h",
            "ui/tempdeviceselector.ui",
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
            "ui/wizard/retroarchpage.cpp",
            "ui/wizard/retroarchpage.h",
            "ui/wizard/retroarchpage.ui",
            "ui/wizard/sd2snespage.cpp",
            "ui/wizard/sd2snespage.h",
            "ui/wizard/sd2snespage.ui",
            "utils/ipsparse.h",
            "utils/ipsparse.cpp",
            "utils/rommapping/mapping_hirom.c",
            "utils/rommapping/mapping_lorom.c",
            "utils/rommapping/rominfo.c",
            "utils/rommapping/rominfo.h",
            "utils/rommapping/rommapping.c",
            "utils/rommapping/rommapping.h",
            "main.cpp",
            "qusb2snes.rc",
            "ressources.qrc",
            "backward.hpp",
            "devices/deviceerror.cpp",
            "devices/deviceerror.h",
            "devices/EmuNWAccess-qt/emunwaccessclient.cpp",
            "devices/EmuNWAccess-qt/emunwaccessclient.h",
            "devices/emunetworkaccessdevice.cpp",
            "devices/emunetworkaccessdevice.h",
            "devices/emunetworkaccessfactory.cpp",
            "devices/emunetworkaccessfactory.h",
            "devices/retroarchfactory.cpp",
            "devices/retroarchfactory.h",
            "devices/retroarchhost.cpp",
            "devices/retroarchhost.h",
            "devices/retroarchdevice.h",
            "devices/retroarchdevice.cpp",
            "devices/sd2snesfactory.cpp",
            "devices/sd2snesfactory.h",
            "devices/snesclassicfactory.cpp",
            "devices/snesclassicfactory.h",
            "devices/luabridge.cpp",
            "devices/luabridge.h",
            "devices/luabridgedevice.cpp",
            "devices/luabridgedevice.h",
            "devices/sd2snesdevice.cpp",
            "devices/sd2snesdevice.h",
            "devices/snesclassic.cpp",
            "devices/snesclassic.h",
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
